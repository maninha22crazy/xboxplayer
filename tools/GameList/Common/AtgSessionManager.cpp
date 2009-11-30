#include "stdafx.h"
#include "AtgSessionManager.h"
#include "AtgMessageRouter.cpp"

#pragma warning( disable:6295 )
#pragma warning( disable:6011 )

namespace ATG
{
//--------------------------------------------------------------------------------------
// Macro for simplying creating tasks
//--------------------------------------------------------------------------------------
#define PUSH_SESSION_TASK(m,p) {                                        \
    ATG::TaskData* ptd = new ATG::TaskData();                           \
    ptd->dwMessage = m;                                                 \
    ptd->pMessage = p;                                                  \
    ptd->pMsgDestination = this;                                        \
    m_pMessageRouter->Send( (const ATG::TaskData)*ptd ); }              \

//-----------------------------------------------------------------------------
// Name: SessionManagerTaskExecutionThreadProc
// Desc: Thread proc of a background thread used to execute session-related
//       tasks
//-----------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManagerTaskExecutionThreadProc
    <TaskQueueType, MessageRouterType>( VOID* lpThreadParameter )
{
    ATG::SessionManager<TaskQueueType, MessageRouterType>* pSessionMgr = 
        ( ATG::SessionManager<TaskQueueType, MessageRouterType>* )lpThreadParameter;
    
    if( pSessionMgr->m_dwSig != SESSIONMANAGER_SIGNATURE )
    {
        ATG::FatalError( "pSessionManager->GetSignature() != SESSIONMANAGER_SIGNATURE !!\n" );
    }
    
    MessageRouter<TaskQueueType>* pMessageRouter = pSessionMgr->m_pMessageRouter;

    while ( 1 )
    {
        if( pSessionMgr->m_bShutdownBGThreads )
        {
            break;
        }
        
        pMessageRouter->Dispatch();
    }

    return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
// Name: SessionManagerMessageCallback
// Desc: Callback for MessageRouter-routed messages
//-----------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManagerMessageCallback( DWORD dwMessageId,
                                    VOID* pMsg,
                                    VOID* pConsumer )
{
    ATG::SessionManager<TaskQueueType, MessageRouterType>* pSessionMgr = 
        ( ATG::SessionManager<TaskQueueType, MessageRouterType>* )pConsumer;
    
    if( pSessionMgr->m_dwSig != SESSIONMANAGER_SIGNATURE )
    {
        ATG::FatalError( "pSessionManager->GetSignature() != SESSIONMANAGER_SIGNATURE !!\n" );
    }

    // Assert if message area is wrong
    assert( MSG_AREA( dwMessageId ) == MSGAREA_SESSIONS );

    switch( dwMessageId )
    {
    case MSG_SESSION_CREATE:
        pSessionMgr->ProcessCreateSessionMessage( pMsg );
        break;
    case MSG_SESSION_ADDLOCAL:
        pSessionMgr->ProcessAddLocalPlayerMessage( pMsg );
        break;
    case MSG_SESSION_ADDREMOTE:
        pSessionMgr->ProcessAddRemotePlayerMessage( pMsg );
        break;
    case MSG_SESSION_REMOVELOCAL:
        pSessionMgr->ProcessRemoveLocalPlayerMessage( pMsg );
        break;
    case MSG_SESSION_REMOVEREMOTE:
        pSessionMgr->ProcessRemoveRemotePlayerMessage( pMsg );
        break;
    case MSG_SESSION_START:
        pSessionMgr->ProcessStartSessionMessage( pMsg );
        break;
    case MSG_SESSION_MODIFYFLAGS:
        pSessionMgr->ProcessModifySessionFlagsMessage( pMsg );
        break;
    case MSG_SESSION_MODIFYSKILL:
        pSessionMgr->ProcessModifySkillMessage( pMsg );
        break;
    case MSG_SESSION_REGISTERARBITRATION:
        pSessionMgr->ProcessArbitrationRegisterMessage( pMsg );
        break;
    case MSG_SESSION_MIGRATE:
        pSessionMgr->ProcessMigrateHostMessage( pMsg );
        break;
    case MSG_SESSION_WRITESTATS:
        pSessionMgr->ProcessWriteStatsMessage( pMsg );
        break;
    case MSG_SESSION_END:
        pSessionMgr->ProcessEndSessionMessage( pMsg );
        break;
    case MSG_SESSION_DELETE:
        pSessionMgr->ProcessDeleteSessionMessage( pMsg );
        break;
    }

    return;
}

//------------------------------------------------------------------------
// Name: SessionManager()
// Desc: Public constructor
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
SessionManager<TaskQueueType, MessageRouterType>::SessionManager() : 
    m_dwSig( SESSIONMANAGER_SIGNATURE ),
    m_bIsMultithreaded( ( NUM_SESSION_TASK_THREADS ) ? 1 : 0 )
{
    Reset();
    m_pMessageRouter = NULL;

    m_phTaskThreads = NULL;
    if( NUM_SESSION_TASK_THREADS )
    {
        m_phTaskThreads = (HANDLE*)malloc( NUM_SESSION_TASK_THREADS * sizeof( HANDLE ) );
    }

    // If we're multithreaded, then create task execution threads
    if( m_bIsMultithreaded )
    {
        for ( DWORD i=0; i<NUM_SESSION_TASK_THREADS; i++ )
        {
            m_phTaskThreads[ i ] = CreateThread(
                    NULL, // SD
                    0, // initial stack size
                    SessionManagerTaskExecutionThreadProc<TaskQueueType, MessageRouterType>, // thread function
                    (VOID*)this, // thread argument
                    CREATE_SUSPENDED , // creation option
                    NULL // thread identifier 
                );

            if( !m_phTaskThreads[ i ] )
            {
                ATG::FatalError( "CreateThread failed! GetLastError: \n", GetLastError() );                    
            }
        }

        // Set thread processor to processor 0.
        for ( DWORD i=0; i<NUM_SESSION_TASK_THREADS; i++ )
        {
            XSetThreadProcessor( m_phTaskThreads[ i ], 0 );
            ResumeThread( m_phTaskThreads[ i ] );
        }
    }

    // Create a signallable event that we'll use to 
    // signal completion of async XSession* calls
    // and to enforce that the calls are serialized
    const BOOL bManualReset = TRUE; //resettable
    const BOOL bInitialState = TRUE; //signalled

    m_hTaskCompletedEvent = CreateEvent(
                                          NULL, // SD
                                          bManualReset, // reset type
                                          bInitialState, // initial state
                                          NULL// object name
    );

    if( !m_hTaskCompletedEvent )
    {
        ATG::FatalError( "CreateEvemt failed! GetLastError: \n", GetLastError() );                    
    }
}

//------------------------------------------------------------------------
// Name: Reset()
// Desc: Resets this SessionManager instance to its creation state
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::Reset()
{
    m_bIsInitialized        = FALSE;
    m_dwSessionFlags        = 0;
    m_bShutdownBGThreads    = FALSE;
    m_dwOwnerController     = XUSER_MAX_COUNT + 1;
    m_xuidOwner             = INVALID_XUID;
    m_SessionState          = SessionStateNone;
    m_hSession              = INVALID_HANDLE_VALUE;
    m_qwSessionNonce        = 0;
    m_qwTrackingNonce       = 0;
    m_strSessionError       = NULL;
    m_bUsingQoS             = FALSE;
    ZeroMemory( &m_SessionInfo, sizeof( XSESSION_INFO ) );
    ZeroMemory( &m_NewSessionInfo, sizeof( XSESSION_INFO ) );
    ZeroMemory( &m_XOverlapped, sizeof( XOVERLAPPED ) );
}


//------------------------------------------------------------------------
// Name: ~SessionManager()
// Desc: Public destructor
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
SessionManager<TaskQueueType, MessageRouterType>::~SessionManager()
{
    // Instruct background threads to exit
    m_bShutdownBGThreads = TRUE;
    
    // Block till for threads to clean up
    const BOOL bWaitAll = TRUE;
    const DWORD dwWaitPeriod = 10; //block for 10ms

    DWORD dwReturn = WaitForMultipleObjects(
                                              NUM_SESSION_TASK_THREADS,
                                              m_phTaskThreads,
                                              bWaitAll,
                                              dwWaitPeriod );

    switch ( dwReturn )
    {
        case WAIT_OBJECT_0: 
            break; 

        // Time-out occurred
        case WAIT_TIMEOUT:
            ATG::FatalError( "WaitForMultipleObjects timeout reached!\n" );
            break;         
    }          

    // Close handles to background threads
    for ( DWORD i=0; i<NUM_SESSION_TASK_THREADS; i++ )
    {
        CloseHandle( m_phTaskThreads[ i ] );                
    }

    // Close our event for managing overlapped operations
    CloseHandle( m_hTaskCompletedEvent );

    // Unregister MessageRouter callback
    if( m_pMessageRouter )
    {
        m_pMessageRouter->UnregisterCallback( this );
    }
}


//------------------------------------------------------------------------
// Name: Initialize()
// Desc: Initializes this SessionManager instance
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
BOOL SessionManager<TaskQueueType, MessageRouterType>::Initialize
    ( ATG::SessionManagerInitParams<TaskQueueType> initParams )
{
    if( m_bIsInitialized )
    {
        ATG::DebugSpew( "ATG::SessionManager already initialized. Reinitializing..\n" );
    }

    m_dwSessionFlags                = initParams.m_dwSessionFlags;
    m_bIsHost                       = initParams.m_bIsHost;
    m_Slots[SLOTS_TOTALPUBLIC]      = initParams.m_dwMaxPublicSlots;
    m_Slots[SLOTS_TOTALPRIVATE]     = initParams.m_dwMaxPrivateSlots;
    m_Slots[SLOTS_FILLEDPUBLIC]     = 0;
    m_Slots[SLOTS_FILLEDPRIVATE]    = 0;
    m_pMessageRouter                = initParams.m_pMessageRouter;

    assert( m_pMessageRouter != NULL );
    m_pMessageRouter->RegisterCallback( MSGAREA_SESSIONS, 
                                        SessionManagerMessageCallback<TaskQueueType,MessageRouterType>,
                                        this );

    m_bIsInitialized = TRUE;
    return TRUE;
}

//------------------------------------------------------------------------
// Name: IsInitialized()
// Desc: Queries SessionManager initialization state
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
BOOL SessionManager<TaskQueueType, MessageRouterType>::IsInitialized()
{
    return m_bIsInitialized;
}

//------------------------------------------------------------------------
// Name: InitializeOverlapped
// Desc: Initializes the m_XOverlapped we'll be using for XSession* calls
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::InitializeOverlapped()
{
    // Close existing m_hTaskCompletedEvent
    CloseHandle( m_hTaskCompletedEvent );
    m_hTaskCompletedEvent = NULL;

    // Create a signallable event that we'll use to 
    // signal completion of async XSession* calls
    // and to enforce that the calls are serialized
    const bool bManualReset = true; //resettable
    const bool bInitialState = false; //non-signalled

    m_hTaskCompletedEvent = CreateEvent(
      NULL, // SD
      bManualReset, // reset type
      bInitialState, // initial state
      NULL// object name
    );

    if( !m_hTaskCompletedEvent )
    {
        ATG::FatalError( "CreateEvemt failed! GetLastError: \n", GetLastError() );                    
    }

    // Set up our overlapped structure with this event
    ZeroMemory( &m_XOverlapped, sizeof( XOVERLAPPED ) );
    m_XOverlapped.hEvent = m_hTaskCompletedEvent;
}

//------------------------------------------------------------------------
// Name: ValidityCheck()
// Desc: Returns TRUE if this is a valid instance; otherwise false
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
BOOL SessionManager<TaskQueueType, MessageRouterType>::ValidityCheck( VOID ) const
{
    return ( m_dwSig == SESSIONMANAGER_SIGNATURE ) ? TRUE : FALSE;
}

//------------------------------------------------------------------------
// Name: IsProcessingOverlappedOperation()
// Desc: Returns true if this instance is in the middle of 
//       processing an overlapped operation; otherwise false
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
BOOL SessionManager<TaskQueueType, MessageRouterType>::IsProcessingOverlappedOperation() const
{
    DWORD dwRet = WaitForSingleObject( m_hTaskCompletedEvent, XOVERLAPPED_TIMEOUT );
    switch( dwRet )
    {
    case WAIT_OBJECT_0: // event is signalled
        return FALSE;
    case WAIT_TIMEOUT: // event is non-signalled
        return TRUE;
    default:
        ATG::FatalError( "Unexpected return value from WaitForSingleObject: %d\n", dwRet );
        return TRUE;
    }
}

//------------------------------------------------------------------------
// Name: GetOverlappedExtendedError()
// Desc: Returns the extended error info from the last completed
//       overlapped operation
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::GetOverlappedExtendedError() const
{
    return XGetOverlappedExtendedError( (XOVERLAPPED *)&m_XOverlapped );
}

//------------------------------------------------------------------------
// Name: DebugDumpSessionDetails()
// Desc: Dumps session deletes
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::DebugDumpSessionDetails()
{
    #ifdef _DEBUG

    m_csSession.Lock();

    if( m_hSession == INVALID_HANDLE_VALUE )
    {
        ATG::DebugSpew( "Session handle invalid. Perhaps session has been deleted?\n" );
        return;
    }

    // Call XSessionGetDetails first time to get back the size 
    // of the results buffer we need
    DWORD cbResults = 0;

    DWORD ret = XSessionGetDetails( m_hSession,
                                    &cbResults,
                                    NULL,
                                    NULL ); 

    if( ( ret != ERROR_INSUFFICIENT_BUFFER ) || ( cbResults == 0 ) )
    {
        ATG::FatalError( "Failed on first call to XSessionGetDetails, hr=0x%08x\n", ret );
    }
    
    XSESSION_LOCAL_DETAILS* pSessionDetails = (XSESSION_LOCAL_DETAILS*)malloc( cbResults * sizeof( XSESSION_LOCAL_DETAILS ) );
    if( pSessionDetails == NULL )
    {
        ATG::FatalError( "Failed to allocate buffer.\n" );
    }

    // Set up our overlapped structure with this event
    ZeroMemory( &m_XOverlapped, sizeof( XOVERLAPPED ) );
    m_XOverlapped.hEvent = m_hTaskCompletedEvent;   

    // Call second time to fill our results buffer
    ret = XSessionGetDetails( m_hSession,
                              &cbResults,
                              pSessionDetails,
                              NULL ); 

    if( ret != ERROR_SUCCESS )
    {
        ATG::FatalError( "XSessionGetDetails failed with error %d\n", ret );
    }

    // Iterate through the returned results
    XNKID sessionID = pSessionDetails->sessionInfo.sessionID;
    ATG::DebugSpew( "***************** DebugDumpSessionDetails *****************\n" \
                    "instance: %p\n" \
                    "sessionID: %016I64X\n" \
                    "sessionstate: %d\n" \
                    "qwSessionNonce(host): %I64u\n" \
                    "qwTrackingNonce (non-host): %I64u\n" \
                    "dwUserIndexHost: %d\n" \
                    "dwGameType: %d\n" \
                    "dwGameMode: %d\n" \
                    "dwFlags: %d\n" \
                    "dwMaxPublicSlots: %d\n" \
                    "dwMaxPrivateSlots: %d\n" \
                    "dwAvailablePublicSlots: %d\n" \
                    "dwAvailablePrivateSlots: %d\n" \
                    "dwActualMemberCount: %d\n" \
                    "dwReturnedMemberCount: %d\n" \
                    "xnkidArbitration: %I64u\n",
                    this,
                    sessionID,
                    m_SessionState,
                    pSessionDetails->qwNonce,
                    m_qwTrackingNonce,
                    pSessionDetails->dwUserIndexHost,
                    pSessionDetails->dwGameType,
                    pSessionDetails->dwGameMode,
                    pSessionDetails->dwFlags,
                    pSessionDetails->dwMaxPublicSlots,
                    pSessionDetails->dwMaxPrivateSlots,
                    pSessionDetails->dwAvailablePublicSlots,
                    pSessionDetails->dwAvailablePrivateSlots,
                    pSessionDetails->dwActualMemberCount,
                    pSessionDetails->dwReturnedMemberCount,
                    pSessionDetails->xnkidArbitration );

    for ( DWORD i = 0; i < pSessionDetails->dwReturnedMemberCount; i++ )
    {
        ATG::DebugSpew( "***************** XSESSION_MEMBER %d *****************\n" \
                        "Online XUID: %I64u\n" \
                        "dwUserIndex: %d\n" \
                        "dwFlags: %d\n",
                        i,
                        pSessionDetails->pSessionMembers->xuidOnline,
                        pSessionDetails->pSessionMembers->dwUserIndex,
                        pSessionDetails->pSessionMembers->dwFlags );    
    }

    m_csSession.Unlock();

    free( pSessionDetails );

    #endif
}


//------------------------------------------------------------------------
// Name: GetSessionState()
// Desc: Retrieves current session state
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
SessionState SessionManager<TaskQueueType, MessageRouterType>::GetSessionState( VOID )
{
    SessionState actual;
    
    m_csSession.Lock();
    actual = m_SessionState;
    m_csSession.Unlock();
    
    return actual;        
}

//------------------------------------------------------------------------
// Name: SetSessionOwner()
// Desc: Sets current session owner
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::SetSessionOwner( DWORD dwOwnerController )
{
    m_csSession.Lock();
    m_dwOwnerController = dwOwnerController;
    XUserGetXUID( m_dwOwnerController, &m_xuidOwner );
    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: GetSessionOwner()
// Desc: Retrieves current session owner
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::GetSessionOwner( VOID )
{
    DWORD dwOwner;
    m_csSession.Lock();
    dwOwner = m_dwOwnerController;
    m_csSession.Unlock();

    return dwOwner;
}

//------------------------------------------------------------------------
// Name: GetSessionOwner()
// Desc: Retrieves current session owner XUID
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
XUID SessionManager<TaskQueueType, MessageRouterType>::GetSessionOwnerXuid( VOID )
{
    XUID xuidOwner;
    m_csSession.Lock();
    xuidOwner = m_xuidOwner;
    m_csSession.Unlock();

    return xuidOwner;
}

//------------------------------------------------------------------------
// Name: IsSessionOwner()
// Desc: Retrieves current session owner
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
BOOL SessionManager<TaskQueueType, MessageRouterType>::IsSessionOwner( DWORD dwController )
{
    BOOL bIsOwner = FALSE;
    m_csSession.Lock();
    bIsOwner = ( dwController == m_dwOwnerController );
    m_csSession.Unlock();

    return bIsOwner;
}

//------------------------------------------------------------------------
// Name: IsSessionOwner()
// Desc: Retrieves current session owner
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
BOOL SessionManager<TaskQueueType, MessageRouterType>::IsSessionOwner( XUID xuidOwner )
{
    BOOL bIsOwner = FALSE;
    m_csSession.Lock();
    bIsOwner = ( xuidOwner == m_xuidOwner );
    m_csSession.Unlock();

    return bIsOwner;
}

//------------------------------------------------------------------------
// Name: IsSessionHost()
// Desc: True if this SessionManager instance is the session host,
//       else false
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
BOOL SessionManager<TaskQueueType, MessageRouterType>::IsSessionHost( VOID )
{
    BOOL bIsHost = FALSE;

    m_csSession.Lock();
    bIsHost = m_bIsHost;
    m_csSession.Unlock();

    return bIsHost;
}

//------------------------------------------------------------------------
// Name: MakeSessionHost()
// Desc: Make this SessionManager instance the session host. 
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::MakeSessionHost( VOID )
{
    m_csSession.Lock();
    m_bIsHost = TRUE;
    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: GetSessionNonce()
// Desc: Function to retrieve current session nonce
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
ULONGLONG SessionManager<TaskQueueType, MessageRouterType>::GetSessionNonce( VOID )
{
    ULONGLONG nonce = 0;

    m_csSession.Lock();
    nonce = ( m_qwSessionNonce ) ? m_qwSessionNonce : m_qwTrackingNonce;
    m_csSession.Unlock();

    return nonce;
}

//------------------------------------------------------------------------
// Name: SetTrackingNonce()
// Desc: Function to set the tracking nonce of the session
//       This is the nonce that's communicated externally
//       when the session's nonce is 0, indicated that this
//       SessionManager instance is not the session host
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::SetTrackingNonce( ULONGLONG qwNonce )
{
    m_csSession.Lock();
    m_qwTrackingNonce = qwNonce;
    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: SetHostInAddr()
// Desc: Sets the IN_ADDR of the session host
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::SetHostInAddr( const IN_ADDR& inaddr )
{
    m_HostInAddr = inaddr;
}

//------------------------------------------------------------------------
// Name: GetHostInAddr()
// Desc: Returns the IN_ADDR of the session host
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
IN_ADDR SessionManager<TaskQueueType, MessageRouterType>::GetHostInAddr( VOID ) const
{
    return m_HostInAddr;
}


//------------------------------------------------------------------------
// Name: GetSessionID()
// Desc: Function to retrieve current session ID
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
XNKID SessionManager<TaskQueueType, MessageRouterType>::GetSessionID( VOID )
{
    XNKID id;
    m_csSession.Lock();
    id = m_SessionInfo.sessionID;
    m_csSession.Unlock();

    return id;
}

//------------------------------------------------------------------------
// Name: SetSessionState()
// Desc: Sets current session state
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::SetSessionState( SessionState newState )
{
    m_csSession.Lock();
    m_SessionState = newState;
    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: CheckSessionState()
// Desc: Checks if session in the expected state
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
BOOL SessionManager<TaskQueueType, MessageRouterType>::CheckSessionState( SessionState expectedState )
{
    BOOL retVal = FALSE;

    m_csSession.Lock();

    SessionState actual = GetSessionState();

    if( actual != expectedState )
    {
        ATG::FatalError( "SessionManager instance in invalid state. Expected: %d; Got: %d\n", 
                          expectedState, actual );

        retVal = FALSE;
    }

    retVal = TRUE;

    m_csSession.Unlock();
    return retVal;
}

//------------------------------------------------------------------------
// Name: GetRegistrationResults()
// Desc: Retrieves last results from registering this client for
//          arbitratrion
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
const PXSESSION_REGISTRATION_RESULTS SessionManager<TaskQueueType, MessageRouterType>::GetRegistrationResults()
{
    PXSESSION_REGISTRATION_RESULTS p = NULL;
    
    m_csSession.Lock();
    p = m_pRegistrationResults;
    m_csSession.Unlock();

    return (const PXSESSION_REGISTRATION_RESULTS)p;    
}

//------------------------------------------------------------------------
// Name: SwitchToState()
// Desc: Changes to a new session state and performs initialization 
//       for the new state
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::SwitchToState( SessionState newState )
{
    m_csSession.Lock();

    ATG::DebugSpew( "SessionManager %016I64X (instance %p): Switching from session state 0x%08x to 0x%08x\n", 
                     m_SessionInfo.sessionID, this, m_SessionState, newState );

    // Clean up from the previous state
    switch( m_SessionState )
    {
        case SessionStateDeleted:
            //Reset();

            // Clear out our slots and update our presence info
            m_Slots[ SLOTS_FILLEDPUBLIC  ] = 0;
            m_Slots[ SLOTS_FILLEDPRIVATE ] = 0;

            if( m_pRegistrationResults )
            {
                free( m_pRegistrationResults );
                m_pRegistrationResults = NULL;
            }
            break;

        case SessionStateMigrateHost:
            // Copy m_NewSessionInfo into m_SessionInfo
            // and zero out m_NewSessionInfo
            memcpy( &m_SessionInfo, &m_NewSessionInfo, sizeof( XSESSION_INFO ) ); 
            ZeroMemory( &m_NewSessionInfo, sizeof( XSESSION_INFO ) );
            break;

        case SessionStateRegistering:
        case SessionStateInGame:
        case SessionStateNone:
        case SessionStateCreating:
        case SessionStateWaitingForRegistration:
        case SessionStateStarting:
        case SessionStateEnding:
        case SessionStateDeleting:
        default:
            //ATG::FatalError( "Attempting to switch to unknown state. Current: %d; Attempting: %d\n", 
            //                  m_SessionState, newState );
            break;
    }

    // Initialize the next state
    switch( newState )
    {
        case SessionStateCreate:
            CreateSession();
            break;

        case SessionStateRegister:
            RegisterArbitration();
            break;

        case SessionStateStart:
            StartSession();
            break;

        case SessionStateEnd:
            EndSession();
            break;

        case SessionStateMigrateHost:
            MigrateSession();
            break;

        case SessionStateDelete:
            StopQoSListener();
            DeleteSession();
            break;

        case SessionStateDeleted:
            CloseHandle( m_hSession );
            break;

        case SessionStateNone:
        case SessionStateWaitingForRegistration:
        case SessionStateRegistered:
        case SessionStateInGame:
        default:
            break;
    }

    m_SessionState = newState;

    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: HasSessionFlags()
// Desc: Checks if passed-in flags are set for the current session
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
BOOL SessionManager<TaskQueueType, MessageRouterType>::HasSessionFlags( DWORD dwFlags )
{
    BOOL bHasFlags = FALSE;
    
    m_csSession.Lock();

    // What flags in m_dwSessionFlags and dwFlags are different?
    DWORD dwDiffFlags = m_dwSessionFlags ^ dwFlags;

    // If none of dwDiffFlags are in dwFlags,
    // we have a match
    if( ( dwDiffFlags & dwFlags ) == 0 )
    {
        bHasFlags = TRUE;
    }

    m_csSession.Unlock();

    return bHasFlags;
}

//------------------------------------------------------------------------
// Name: FlipSessionFlags()
// Desc: XORs the state of the current session's
//       flags with that of the passed-in flags
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::FlipSessionFlags( DWORD dwFlags )
{ 
    m_csSession.Lock();

    m_dwSessionFlags ^= dwFlags;

    ATG::DebugSpew( "FlipSessionFlags: New session flags: %d\n", m_dwSessionFlags );

    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: ClearSessionFlags()
// Desc: Clear the passed-in flags for the
//       current session
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::ClearSessionFlags( DWORD dwFlags )
{ 
    m_csSession.Lock();

    m_dwSessionFlags &= ~dwFlags; 

    ATG::DebugSpew( "ClearSessionFlags: New session flags: %d\n", m_dwSessionFlags );

    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: SetSessionInfo()
// Desc: Set session info data. This only be called outside of an
//       active session
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::SetSessionInfo( const XSESSION_INFO& session_info )
{
    m_csSession.Lock();

    SessionState actual = GetSessionState();
    if( m_SessionState > SessionStateCreating && m_SessionState < SessionStateDelete )
    {
        ATG::FatalError( "SessionManager instance in invalid state: %d\n", actual );
    }

    memcpy( &m_SessionInfo, &session_info, sizeof( XSESSION_INFO ) );

    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: SetNewSessionInfo()
// Desc: Set session info data for the new session we will migrate this
//       session to. This only be called inside of an active session
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::SetNewSessionInfo( const XSESSION_INFO& session_info )
{
    m_csSession.Lock();

    SessionState actual = GetSessionState();
    if( m_SessionState < SessionStateCreated || m_SessionState >= SessionStateDelete )
    {
        ATG::FatalError( "SessionManager instance in invalid state: %d\n", actual );
    }

    memcpy( &m_NewSessionInfo, &session_info, sizeof( XSESSION_INFO ) );

    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: SetSessionFlags()
// Desc: Set session flags. Optionally first clears all currently set 
//       flags. This only be called outside of an active session
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::SetSessionFlags( DWORD dwFlags, BOOL fClearExisting  )
{
    m_csSession.Lock();

    SessionState actual = GetSessionState();
    if( m_SessionState > SessionStateCreating && m_SessionState < SessionStateDelete )
    {
        ATG::FatalError( "SessionManager instance in invalid state: %d\n", actual );
    }

    if( fClearExisting )
    {
        m_dwSessionFlags = 0;
    }

    m_dwSessionFlags |= dwFlags;

    ATG::DebugSpew( "SetSessionFlags: New session flags: %d\n", m_dwSessionFlags );

    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: GetSessionFlags()
// Desc: Returns current session flags
// Thread Safe: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::GetSessionFlags()
{
    DWORD dwFlags = 0;

    m_csSession.Lock();
    dwFlags = m_dwSessionFlags;
    m_csSession.Unlock();

    return dwFlags;
}

//------------------------------------------------------------------------
// Name: GetSessionInfo()
// Desc: Returns current XSESSION_INFO
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
const XSESSION_INFO& SessionManager<TaskQueueType, MessageRouterType>::GetSessionInfo() const
{
    return m_SessionInfo;
}

//------------------------------------------------------------------------
// Name: GetMaxSlotCounts()
// Desc: Returns current maximum slot counts
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::GetMaxSlotCounts( DWORD& dwMaxPublicSlots, DWORD& dwMaxPrivateSlots )
{
    m_csSession.Lock();

    dwMaxPublicSlots  = m_Slots[SLOTS_TOTALPUBLIC];
    dwMaxPrivateSlots = m_Slots[SLOTS_TOTALPRIVATE];

    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: GetFilledSlotCounts()
// Desc: Returns current filled slot counts
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::GetFilledSlotCounts( DWORD& dwFilledPublicSlots, DWORD& dwFilledPrivateSlots )
{
    m_csSession.Lock();

    dwFilledPublicSlots  = m_Slots[SLOTS_FILLEDPUBLIC];
    dwFilledPrivateSlots = m_Slots[SLOTS_FILLEDPRIVATE];

    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: SetMaxSlotCounts()
// Desc: Sets current maximum slot counts
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::
    SetMaxSlotCounts( DWORD dwMaxPublicSlots, DWORD dwMaxPrivateSlots )
{
    m_csSession.Lock();

    m_Slots[SLOTS_TOTALPUBLIC]  = dwMaxPublicSlots;
    m_Slots[SLOTS_TOTALPRIVATE] = dwMaxPrivateSlots;

    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: SetFilledSlotCounts()
// Desc: Sets current filled slot counts
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::
    SetFilledSlotCounts( DWORD dwFilledPublicSlots, DWORD dwFilledPrivateSlots )
{
    m_csSession.Lock();

    m_Slots[SLOTS_FILLEDPUBLIC]  = dwFilledPublicSlots;
    m_Slots[SLOTS_FILLEDPRIVATE] = dwFilledPrivateSlots;

    m_csSession.Unlock();
}

//------------------------------------------------------------------------
// Name: GetSessionError()
// Desc: Returns session error string
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
WCHAR* SessionManager<TaskQueueType, MessageRouterType>::GetSessionError()
{
    return m_strSessionError;
}

//------------------------------------------------------------------------
// Name: SetSessionError()
// Desc: Sets session error string
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::SetSessionError( WCHAR* error )
{
    m_strSessionError = error;
}


//--------------------------------------------------------------------------------------
// Name: StartQoSListener()
// Desc: Turn on the Quality of Service Listener for the current session
// Thread Safety: Required
//--------------------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::StartQoSListener( BYTE* data, UINT dataLen, DWORD bitsPerSec )
{
    m_csSession.Lock();

    DWORD flags = XNET_QOS_LISTEN_SET_DATA;
    if( bitsPerSec )
    {
        flags |= XNET_QOS_LISTEN_SET_BITSPERSEC;
    }
    if( !m_bUsingQoS )
    {
        flags |= XNET_QOS_LISTEN_ENABLE;
    }
    DWORD dwRet;
    dwRet = XNetQosListen( &( m_SessionInfo.sessionID ), data, dataLen, bitsPerSec, flags );
    if( ERROR_SUCCESS != dwRet )
    {
        ATG::FatalError( "Failed to start QoS listener, error 0x%08x\n", dwRet );
    }
    m_bUsingQoS = TRUE;

    m_csSession.Unlock();
}


//--------------------------------------------------------------------------------------
// Name: StopQoSListener()
// Desc: Turn off the Quality of Service Listener for the current session
// Thread Safety: Required
//--------------------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
VOID SessionManager<TaskQueueType, MessageRouterType>::StopQoSListener()
{
    m_csSession.Lock();

    if( m_bUsingQoS )
    {
        DWORD dwRet;
        dwRet = XNetQosListen( &( m_SessionInfo.sessionID ), NULL, 0, 0,
                               XNET_QOS_LISTEN_RELEASE );
        if( ERROR_SUCCESS != dwRet )
        {
            ATG::DebugSpew( "Warning: Failed to stop QoS listener, error 0x%08x\n", dwRet );
        }
        m_bUsingQoS = FALSE;
    }

    m_csSession.Unlock();
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
// The following methods push messages on the task queue that may be processing 
// on the same thread on which this SessionManager instance was created, 
// or on a background thread if the NUM_SESSION_TASK_THREADS value is not 0
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

//------------------------------------------------------------------------
// Name: CreateSession()
// Desc: Creates a session
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
HRESULT SessionManager<TaskQueueType, MessageRouterType>::CreateSession( )

{
    // Make sure our instance is in the right state
    SessionState actual = GetSessionState();
    if( actual != SessionStateNone && actual != SessionStateDeleted  )
    {
        // $TODO - FatalError instead once it's understood why
        // the is called twice in certain edge cases
        ATG::DebugSpew( "Wrong state: %d\n", actual );
        return S_OK;
    }

    // Modify session flags if host
    if( m_bIsHost )
    {
        m_dwSessionFlags |= XSESSION_CREATE_HOST;
    }
    else
    {
        // Make sure the host bit is set..
        m_dwSessionFlags |= XSESSION_CREATE_HOST;

        // ..then toggle it off
        m_dwSessionFlags ^= XSESSION_CREATE_HOST;
    }

    // Set up our message
    DWORD dwMessage             = MSG_SESSION_CREATE;
    SessionCreateMsg* pMsg      = new SessionCreateMsg();
    pMsg->dwFlags               = m_dwSessionFlags;
    pMsg->dwUserIndex           = m_dwOwnerController;
    pMsg->dwMaxPrivateSlots     = m_Slots[SLOTS_TOTALPUBLIC];
    pMsg->dwMaxPublicSlots      = m_Slots[SLOTS_TOTALPRIVATE];

    // Set up our task
    PUSH_SESSION_TASK( dwMessage, pMsg );

    // Return ERROR_IO_PENDING since we're by design async 
    return HRESULT_FROM_WIN32( ERROR_IO_PENDING );
}

//------------------------------------------------------------------------
// Name: StartSession()
// Desc: Starts a session
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
HRESULT SessionManager<TaskQueueType, MessageRouterType>::StartSession()
{
    // Make sure our instance is in the right state
    SessionState actual = GetSessionState();
    if( actual != SessionStateRegistered && actual != SessionStateCreated )
    {
        ATG::FatalError( "Wrong state: %d\n", actual );
    }

    // Set up our message
    DWORD dwMessage             = MSG_SESSION_START;
    SessionStartMsg* pMsg       = new SessionStartMsg();

    // Set up our task
    PUSH_SESSION_TASK( dwMessage, pMsg );

    // Return ERROR_IO_PENDING since we're by design async 
    return HRESULT_FROM_WIN32( ERROR_IO_PENDING );
}

//------------------------------------------------------------------------
// Name: EndSession()
// Desc: Ends the current session
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
HRESULT SessionManager<TaskQueueType, MessageRouterType>::EndSession()
{
    // Make sure our instance is in the right state. If not,
    // do nothing
    SessionState actual = GetSessionState();
    if( actual != SessionStateInGame && actual != SessionStateStarting )
    {
        ATG::DebugSpew( "SessionManager instance in invalid state. Ignoring EndSession %d\n", 
                        actual );
        return S_OK;
    }

    // Set up our message
    DWORD dwMessage     = MSG_SESSION_END;
    SessionEndMsg* pMsg = new SessionEndMsg();

    // Set up our task
    PUSH_SESSION_TASK( dwMessage, pMsg );

    // Return ERROR_IO_PENDING since we're by design async 
    return HRESULT_FROM_WIN32( ERROR_IO_PENDING );
}

//------------------------------------------------------------------------
// Name: MigrateSession()
// Desc: Migrates the current session
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
HRESULT SessionManager<TaskQueueType, MessageRouterType>::MigrateSession()
{
    // Make sure our instance is in the right state
    SessionState actual = GetSessionState();
    if( actual < SessionStateCreated || actual >= SessionStateDelete  )
    {
        ATG::FatalError( "SessionManager instance in invalid state: %d\n", actual );
    }

    // Set up our message
    DWORD dwMessage         = MSG_SESSION_MIGRATE;
    SessionMigrateMsg* pMsg = new SessionMigrateMsg();

    // Set up our task
    PUSH_SESSION_TASK( dwMessage, pMsg );

    // Return ERROR_IO_PENDING since we're by design async 
    return HRESULT_FROM_WIN32( ERROR_IO_PENDING );
}

//------------------------------------------------------------------------
// Name: ModifySessionFlags()
// Desc: Modifies session flags
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ModifySessionFlags( DWORD dwFlags )
{
    m_csSession.Lock();

    SessionState actual = GetSessionState();
    if( m_SessionState <= SessionStateCreating || m_SessionState >= SessionStateDelete )
    {
        ATG::FatalError( "SessionManager instance in invalid state: %d\n", actual );
    }

    DWORD ret = ERROR_SUCCESS;

    // Set up our message
    DWORD dwMessage             = MSG_SESSION_MODIFYFLAGS;
    SessionModifyFlagsMsg* pMsg = new SessionModifyFlagsMsg();
    pMsg->dwFlags = dwFlags;

    // Set up our task
    PUSH_SESSION_TASK( dwMessage, pMsg );

    // Return ERROR_IO_PENDING since we're by design async 
    ret = HRESULT_FROM_WIN32( ERROR_IO_PENDING );

    m_csSession.Unlock();
    
    return ret;
}

//------------------------------------------------------------------------
// Name: ModifySkill()
// Desc: Modifies TrueSkill(TM)
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
HRESULT SessionManager<TaskQueueType, MessageRouterType>::ModifySkill( const DWORD dwNumXuids, const XUID* pXuids )
{
    m_csSession.Lock();

    SessionState actual = GetSessionState();
    if( m_SessionState <= SessionStateCreating || m_SessionState >= SessionStateDelete )
    {
        ATG::FatalError( "SessionManager instance in invalid state: %d\n", actual );
    }

    DWORD ret = ERROR_SUCCESS;

    // Allocate memory on the heap for passed-in array
    XUID* xuids = (XUID*)malloc( dwNumXuids * sizeof(XUID) );

    if( !xuids )
    {
        ATG::FatalError( "Failed to allocate memory!\n" );
    }

    memcpy( xuids, pXuids, dwNumXuids * sizeof(XUID) );

    // Set up our message
    DWORD dwMessage = MSG_SESSION_MODIFYSKILL;
    ModifySkillMsg* pMsg = new ModifySkillMsg();
    pMsg->dwXuidCount = dwNumXuids;
    pMsg->pXuids = xuids;

    // Set up our task
    PUSH_SESSION_TASK( dwMessage, pMsg );

    // Return ERROR_IO_PENDING since we're by design async 
    ret = HRESULT_FROM_WIN32( ERROR_IO_PENDING );

    m_csSession.Unlock();
    
    return ret;
}

//------------------------------------------------------------------------
// Name: RegisterArbitration()
// Desc: Registers the current session for arbitration
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
HRESULT SessionManager<TaskQueueType, MessageRouterType>::RegisterArbitration()
{
    m_csSession.Lock();

    SessionState actual = GetSessionState();
    if( actual < SessionStateCreated || actual >= SessionStateStarting )
    {
        ATG::FatalError( "Wrong state: %d\n", actual );
    }

    DWORD ret = ERROR_SUCCESS;

    // Set up our message
    DWORD dwMessage                 = MSG_SESSION_REGISTERARBITRATION;
    ArbitrationRegisterMsg* pMsg    = new ArbitrationRegisterMsg();

    // Set up our task
    PUSH_SESSION_TASK( dwMessage, pMsg );

    // Return ERROR_IO_PENDING since we're by design async 
    ret = HRESULT_FROM_WIN32( ERROR_IO_PENDING );

    m_csSession.Unlock();
    
    return ret;
}

//------------------------------------------------------------------------
// Name: DeleteSession()
// Desc: Deletes the current session
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
HRESULT SessionManager<TaskQueueType, MessageRouterType>::DeleteSession()
{
    // Make sure our instance is in the right state
    SessionState actual = GetSessionState();
    if( actual < SessionStateCreated || actual > SessionStateDelete  )
    {
        ATG::FatalError( "Wrong state: %d\n", actual );
    }

    // Set up our message
    DWORD dwMessage         = MSG_SESSION_DELETE;
    SessionDeleteMsg* pMsg  = new SessionDeleteMsg();

    // Set up our task
    PUSH_SESSION_TASK( dwMessage, pMsg );

    // Return ERROR_IO_PENDING since we're by design async 
    return HRESULT_FROM_WIN32( ERROR_IO_PENDING );
}

//------------------------------------------------------------------------
// Name: AddLocalPlayers()
// Desc: Add local players to the session
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
HRESULT SessionManager<TaskQueueType, MessageRouterType>::AddLocalPlayers( const DWORD dwUserCount,
                                         const DWORD* pdwUserIndices,
                                         const BOOL* pfPrivateSlots )
{
    SessionState actual = GetSessionState();
    if( actual < SessionStateCreating || actual >= SessionStateDelete )
    {
        ATG::FatalError( "Wrong state: %d\n", actual );
    }

    // Allocate memory on the heap for passed-in arrays
    DWORD* userIndices = (DWORD*)malloc( dwUserCount * sizeof(DWORD) );
    BOOL* privateSlots = (BOOL*)malloc( dwUserCount * sizeof(BOOL) );

    if( !userIndices || !privateSlots )
    {
        ATG::FatalError( "Failed to allocate memory!\n" );
    }

    memcpy( userIndices, pdwUserIndices, dwUserCount * sizeof(DWORD) );
    memcpy( privateSlots, pfPrivateSlots, dwUserCount * sizeof(BOOL) );

    // Set up our message
    DWORD dwMessage             = MSG_SESSION_ADDLOCAL;
    AddLocalPlayersMsg* pMsg   = new AddLocalPlayersMsg();
    pMsg->dwUserCount           = dwUserCount;
    pMsg->pdwUserIndices        = (const DWORD*)userIndices;
    pMsg->pfPrivateSlots        = (const BOOL*)privateSlots;

    // Set up our task
    PUSH_SESSION_TASK( dwMessage, pMsg );

    // Return ERROR_IO_PENDING since we're by design async 
    return HRESULT_FROM_WIN32( ERROR_IO_PENDING );
}

//------------------------------------------------------------------------
// Name: AddRemotePlayers()
// Desc: Adds remote players to the session
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
HRESULT SessionManager<TaskQueueType, MessageRouterType>::AddRemotePlayers( const DWORD dwXuidCount,
                                          const XUID* pXuids,
                                          const BOOL* pfPrivateSlots )
{
    SessionState actual = GetSessionState();
    if( actual < SessionStateCreating || actual >= SessionStateDelete )
    {
        ATG::FatalError( "Wrong state: %d\n", actual );
    }

    // Allocate memory on the heap for passed-in arrays
    DWORD* xuids = (DWORD*)malloc( dwXuidCount * sizeof(XUID) );
    BOOL* privateSlots = (BOOL*)malloc( dwXuidCount * sizeof(BOOL) );

    if( !xuids || !privateSlots )
    {
        ATG::FatalError( "Failed to allocate memory!\n" );
    }

    memcpy( xuids, pXuids, dwXuidCount * sizeof(XUID) );
    memcpy( privateSlots, pfPrivateSlots, dwXuidCount * sizeof(BOOL) );

    for ( DWORD i=0; i<dwXuidCount; i++ )
    {
        ATG::DebugSpew( "AddRemotePlayers: %I64u; bInvited: %d\n", pXuids[ i ], pfPrivateSlots[ i ] );
    }

    // Set up our message
    DWORD dwMessage             = MSG_SESSION_ADDREMOTE;
    AddRemotePlayersMsg* pMsg   = new AddRemotePlayersMsg();
    pMsg->dwXuidCount           = dwXuidCount;
    pMsg->pXuids                = (const XUID*)xuids;
    pMsg->pfPrivateSlots        = (const BOOL*)privateSlots;

    // Set up our task
    PUSH_SESSION_TASK( dwMessage, pMsg );

    // Return ERROR_IO_PENDING since we're by design async 
    return HRESULT_FROM_WIN32( ERROR_IO_PENDING );
}

//------------------------------------------------------------------------
// Name: RemoveLocalPlayers()
// Desc: Add local players to the session
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
HRESULT SessionManager<TaskQueueType, MessageRouterType>::RemoveLocalPlayers( const DWORD dwUserCount,
                                            const DWORD* pdwUserIndices,
                                            const BOOL* pfPrivateSlots )
{
    SessionState actual = GetSessionState();
    if( actual < SessionStateCreated || actual >= SessionStateDelete )
    {
        ATG::FatalError( "Wrong state: %d\n", actual );
    }

    // Allocate memory on the heap for passed-in arrays
    DWORD* userIndices = (DWORD*)malloc( dwUserCount * sizeof(DWORD) );
    BOOL* privateSlots = (BOOL*)malloc( dwUserCount * sizeof(BOOL) );

    if( !userIndices || !privateSlots )
    {
        ATG::FatalError( "Failed to allocate memory!\n" );
    }

    memcpy( userIndices, pdwUserIndices, dwUserCount * sizeof(DWORD) );
    memcpy( privateSlots, pfPrivateSlots, dwUserCount * sizeof(BOOL) );

    // Set up our message
    DWORD dwMessage                 = MSG_SESSION_REMOVELOCAL;
    RemoveLocalPlayersMsg* pMsg    = new RemoveLocalPlayersMsg();
    pMsg->dwUserCount               = dwUserCount;
    pMsg->pdwUserIndices            = (const DWORD*)userIndices;
    pMsg->pfPrivateSlots            = (const BOOL*)privateSlots;

    // Set up our task
    PUSH_SESSION_TASK( dwMessage, pMsg );

    // Return ERROR_IO_PENDING since we're by design async 
    return HRESULT_FROM_WIN32( ERROR_IO_PENDING );
}

//------------------------------------------------------------------------
// Name: RemoveRemotePlayers()
// Desc: Remove remote players to the session
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
HRESULT SessionManager<TaskQueueType, MessageRouterType>::RemoveRemotePlayers( const DWORD dwXuidCount,
                                             const XUID* pXuids,
                                             const BOOL* pfPrivateSlots )
{
    SessionState actual = GetSessionState();
    if( actual < SessionStateCreated || actual >= SessionStateDelete )
    {
        ATG::FatalError( "Wrong state: %d\n", actual );
    }

    // Allocate memory on the heap for passed-in arrays
    DWORD* xuids = (DWORD*)malloc( dwXuidCount * sizeof(XUID) );
    BOOL* privateSlots = (BOOL*)malloc( dwXuidCount * sizeof(BOOL) );

    if( !xuids || !privateSlots )
    {
        ATG::FatalError( "Failed to allocate memory!\n" );
    }

    memcpy( xuids, pXuids, dwXuidCount * sizeof(XUID) );
    memcpy( privateSlots, pfPrivateSlots, dwXuidCount * sizeof(BOOL) );

    // Set up our message
    DWORD dwMessage                 = MSG_SESSION_REMOVEREMOTE;
    RemoveRemotePlayersMsg* pMsg    = new RemoveRemotePlayersMsg();
    pMsg->dwXuidCount               = dwXuidCount;
    pMsg->pXuids                    = (const XUID*)xuids;
    pMsg->pfPrivateSlots            = (const BOOL*)privateSlots;

    // Set up our task
    PUSH_SESSION_TASK( dwMessage, pMsg );

    // Return ERROR_IO_PENDING since we're by design async 
    return HRESULT_FROM_WIN32( ERROR_IO_PENDING );
}


//------------------------------------------------------------------------
// Name: WriteStats()
// Desc: Write stats for a player in the session
// Thread Safety: Not required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
HRESULT SessionManager<TaskQueueType, MessageRouterType>::WriteStats( XUID xuid,
                                       DWORD dwNumViews,
                                       const XSESSION_VIEW_PROPERTIES *pViews )
{
    SessionState actual = GetSessionState();
    if( actual < SessionStateInGame || actual >= SessionStateEnding )
    {
        ATG::FatalError( "Wrong state: %d\n", actual );
    }

    // Set up our message
    DWORD dwMessage                 = MSG_SESSION_WRITESTATS;
    WriteStatsMsg* pMsg             = new WriteStatsMsg();
    pMsg->xuid                      = xuid;
    pMsg->dwNumViews                = dwNumViews;
    pMsg->pViews                    = pViews;

    // Set up our task
    PUSH_SESSION_TASK( dwMessage, pMsg );

    // Return ERROR_IO_PENDING since we're by design async 
    return HRESULT_FROM_WIN32( ERROR_IO_PENDING );
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
// The following methods may be called either in the same thread on
// which this SessionManager instance was created, or on a background
// thread if the NUM_SESSION_TASK_THREADS value is not 0
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

//------------------------------------------------------------------------
// Name: ProcessCreateSessionMessage()
// Desc: Creates a session using the XSessionCreate API
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ProcessCreateSessionMessage( VOID* pMessage )
{
    m_csSession.Lock();

    SessionCreateMsg* pMsg = ( SessionCreateMsg* )pMessage;

    InitializeOverlapped();
    
    DWORD ret = XSessionCreate( pMsg->dwFlags,
                                pMsg->dwUserIndex,
                                pMsg->dwMaxPrivateSlots,
                                pMsg->dwMaxPrivateSlots,
                                &m_qwSessionNonce,
                                &m_SessionInfo,
                                &m_XOverlapped,
                                &m_hSession );

    if( ret != ERROR_IO_PENDING )
    {
        ATG::FatalError( "XSessionCreate failed with error %d\n", ret );
    }

    SwitchToState( ATG::SessionStateCreating );

    m_csSession.Unlock();

    return ret;
}

//------------------------------------------------------------------------
// Name: ProcessStartSessionMessage()
// Desc: Creates a session using the XSessionStart API
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ProcessStartSessionMessage( VOID* pMessage )
{
    m_csSession.Lock();

    InitializeOverlapped();

    DWORD ret = XSessionStart( m_hSession,
                               0,
                               &m_XOverlapped );

    if( ret != ERROR_IO_PENDING )
    {
        ATG::FatalError( "XSessionStart failed with error %d\n", ret );
    }

    SwitchToState( ATG::SessionStateStarting );
    
    m_csSession.Unlock();

    return ret;
}

//------------------------------------------------------------------------
// Name: ProcessMigrateHostMessage()
// Desc: Migrates session to new host using the XSessionMigrateHost API
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ProcessMigrateHostMessage( VOID* pMessage )
{
    m_csSession.Lock();

    // $TODO - Overlapped XSessionMigrateHost never seems to return, so
    // make this call synchronous and set m_hTaskCompletedEvent
    // to make sure nobody is waiting on it to be signalled..
    if( !SetEvent( m_hTaskCompletedEvent ) )
    {
        ATG::FatalError( "SetEvent failed! GetLastError: \n", GetLastError() );                    
    }

    DWORD ret = XSessionMigrateHost( m_hSession,
                                     m_dwOwnerController,
                                     &m_NewSessionInfo,
                                     NULL );

    if( ret != ERROR_SUCCESS )
    {
        ATG::FatalError( "XSessionMigrateHost failed with error %d\n", ret );
    }

    m_csSession.Unlock();

    DebugDumpSessionDetails();

    return ret;
}

//------------------------------------------------------------------------
// Name: ProcessWriteStatsMessage()
// Desc: Writes stats for the current session
//       using the XSessionWriteStats API
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ProcessWriteStatsMessage( VOID* pMessage )
{
    m_csSession.Lock();
    DWORD ret = ERROR_SUCCESS;

    SessionState actual = GetSessionState();
    if( actual > SessionStateEnding )
    {
        // Can only write stats between XSessionStart and XSessionEnd, so
        // do nothing
        return ret;
    }

    WriteStatsMsg* pMsg                     = ( WriteStatsMsg* )pMessage;
    XUID  xuid                              = pMsg->xuid;
    DWORD dwNumViews                        = pMsg->dwNumViews;
    const XSESSION_VIEW_PROPERTIES* pViews  = pMsg->pViews;

    InitializeOverlapped();

    ret = XSessionWriteStats( m_hSession,
                              xuid,
                              dwNumViews,
                              pViews,
                              &m_XOverlapped );

    if( ret != ERROR_IO_PENDING )
    {
        ATG::FatalError( "XSessionWriteStats failed with error %d\n", ret );
    }

    return ret;
}


//------------------------------------------------------------------------
// Name: ProcessEndSessionMessage()
// Desc: Ends current session using the XSessionEnd API
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ProcessEndSessionMessage( VOID* pMessage )
{
    m_csSession.Lock();

    InitializeOverlapped();

    DWORD ret = XSessionEnd( m_hSession, &m_XOverlapped );

    if( ret != ERROR_IO_PENDING )
    {
        ATG::FatalError( "XSessionEnd failed with error %d\n", ret );
    }

    SwitchToState( ATG::SessionStateEnding );

    m_csSession.Unlock();

    return ret;
}

//------------------------------------------------------------------------
// Name: ProcessDeleteSessionMessage()
// Desc: Deletes current session using the XSessionDelete API
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ProcessDeleteSessionMessage( VOID* pMessage )
{
    SessionState actual = GetSessionState();
    if( actual > SessionStateDelete )
    {
        ATG::FatalError( "Session already in a deleting state: %d\n", actual );
    }

    m_csSession.Lock();

    InitializeOverlapped();

    DWORD ret = XSessionDelete( m_hSession, &m_XOverlapped );

    if( ret != ERROR_IO_PENDING )
    {
        ATG::FatalError( "XSessionDelete failed with error %d\n", ret );
    }

    SwitchToState( ATG::SessionStateDeleting );

    m_csSession.Unlock();

    return ret;
}

//------------------------------------------------------------------------
// Name: ProcessModifySessionFlagsMessage()
// Desc: Modifies a session's flags using the XSessionModify API
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ProcessModifySessionFlagsMessage( VOID* pMessage )
{
    m_csSession.Lock();

    SessionModifyFlagsMsg* pMsg = ( SessionModifyFlagsMsg* )pMessage;

    InitializeOverlapped();

    // turn the allowed modify flags off
    m_dwSessionFlags &= ~MODIFY_FLAGS_ALLOWED;

    // and get them from the flags input
    m_dwSessionFlags |= ( pMsg->dwFlags & ATG::MODIFY_FLAGS_ALLOWED );

    ATG::DebugSpew( "ProcessModifySessionFlagsMessage: New session flags: %d\n", m_dwSessionFlags );

    DWORD ret = XSessionModify( m_hSession,
                                m_dwSessionFlags,
                                m_Slots[SLOTS_TOTALPUBLIC],
                                m_Slots[SLOTS_TOTALPRIVATE],
                                &m_XOverlapped );

    if( ret != ERROR_IO_PENDING )
    {
        ATG::FatalError( "XSessionModify failed with error %d\n", ret );
    }

    // Wait for the overlapped operation to complete
    while ( IsProcessingOverlappedOperation() );

    HRESULT hr = HRESULT_FROM_WIN32( XGetOverlappedExtendedError( &m_XOverlapped ) );
    if( !SUCCEEDED( hr ) )
    {
        ATG::FatalError( "XGetOverlappedExtendedError returned error 0x%08x\n", hr );
    }

    m_csSession.Unlock();

    return ret;
}

//------------------------------------------------------------------------
// Name: ProcessArbitrationRegisterMessage()
// Desc: Deletes current session using the XSessionArbitrationRegister API
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ProcessArbitrationRegisterMessage( VOID* pMessage )
{
    m_csSession.Lock();

    // Call once to determine size of results buffer
    DWORD cbRegistrationResults = 0;

    DWORD ret = XSessionArbitrationRegister( m_hSession,
                                             0,
                                             m_qwSessionNonce,
                                             &cbRegistrationResults,
                                             NULL,
                                             NULL ); 

    if( ( ret != ERROR_INSUFFICIENT_BUFFER ) || ( cbRegistrationResults == 0 ) )
    {
        ATG::FatalError( "Failed on first call to XSessionArbitrationRegister, hr=0x%08x\n", ret );
    }

    m_pRegistrationResults = (PXSESSION_REGISTRATION_RESULTS)malloc( cbRegistrationResults * sizeof( XSESSION_REGISTRATION_RESULTS ) );
    if( m_pRegistrationResults == NULL )
    {
        ATG::FatalError( "Failed to allocate buffer.\n" );
    }

    InitializeOverlapped();

    // Call second time to fill our results buffer
    ret = XSessionArbitrationRegister( m_hSession,
                                       0,
                                       m_qwSessionNonce,
                                       &cbRegistrationResults,
                                       m_pRegistrationResults,
                                       &m_XOverlapped ); 

    if( ret != ERROR_IO_PENDING )
    {
        ATG::FatalError( "XSessionArbitrationRegister failed with error %d\n", ret );
    }

    SwitchToState( ATG::SessionStateRegistering );

    m_csSession.Unlock();

    return ret;
}

//------------------------------------------------------------------------
// Name: ProcessAddLocalPlayerMessage()
// Desc: Add local players using the XSessionJoinLocal API
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ProcessAddLocalPlayerMessage( VOID* pMessage )
{
    m_csSession.Lock();
    DWORD ret = ERROR_SUCCESS;

    AddLocalPlayersMsg* pMsg    = ( AddLocalPlayersMsg* )pMessage;
    DWORD dwUserCount           = pMsg->dwUserCount;
    const DWORD* pdwUserIndices = pMsg->pdwUserIndices;
    const BOOL* pfPrivateSlots  = pMsg->pfPrivateSlots;

    InitializeOverlapped();

    ret = XSessionJoinLocal( m_hSession,
                             dwUserCount,
                             pdwUserIndices,
                             pfPrivateSlots,
                             &m_XOverlapped );

    if( ret != ERROR_IO_PENDING )
    {
        ATG::FatalError( "XSessionJoinLocal failed with error %d\n", ret );
    }

    // Wait for the overlapped operation to complete
    while ( IsProcessingOverlappedOperation() );

    // Make sure we were able to add local users to the
    // session and then update slot counts
    ret = XGetOverlappedExtendedError( &m_XOverlapped );
    if( ret != ERROR_SUCCESS )
    {
        ATG::FatalError( "Failed to add local users to session, error 0x%08x\n", HRESULT_FROM_WIN32( ret ) );
    }
    else
    {
        for ( DWORD i=0; i<dwUserCount; i++ )
        {
            m_Slots[ SLOTS_FILLEDPRIVATE ] += ( pfPrivateSlots[ i ] ) ? 1 : 0;
            m_Slots[ SLOTS_FILLEDPUBLIC ] += ( pfPrivateSlots[ i ] ) ? 0 : 1;
        }

        if( m_Slots[ SLOTS_FILLEDPRIVATE ] > m_Slots[ SLOTS_TOTALPRIVATE ] )
        {
            m_Slots[ SLOTS_FILLEDPRIVATE ] = m_Slots[ SLOTS_TOTALPRIVATE ];

            DWORD diff = m_Slots[ SLOTS_FILLEDPRIVATE ] - m_Slots[ SLOTS_TOTALPRIVATE ];
            m_Slots[ SLOTS_FILLEDPUBLIC ] += diff;

            assert( m_Slots[ SLOTS_FILLEDPUBLIC ] <= m_Slots[ SLOTS_TOTALPUBLIC ] );
            if( m_Slots[ SLOTS_FILLEDPUBLIC ] > m_Slots[ SLOTS_TOTALPUBLIC ] )
            {
                ATG::FatalError( "Too many slots filled!\n" );
            }
        }
    }

    // Free memory associated with pMsg
    delete pMsg;

    DebugDumpSessionDetails();

    m_csSession.Unlock();

    return ret;
}

//------------------------------------------------------------------------
// Name: ProcessAddRemotePlayerMessage()
// Desc: Add local players using the XSessionJoinRemote API
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ProcessAddRemotePlayerMessage( VOID* pMessage )
{
    m_csSession.Lock();
    DWORD ret = ERROR_SUCCESS;

    AddRemotePlayersMsg* pMsg   = ( AddRemotePlayersMsg* )pMessage;
    DWORD dwXuidCount           = pMsg->dwXuidCount;
    const XUID* pXuids          = pMsg->pXuids;
    const BOOL* pfPrivateSlots  = pMsg->pfPrivateSlots;

    for ( DWORD i=0; i<dwXuidCount; i++ )
    {
        ATG::DebugSpew( "Processing remote user: %I64u; bInvited: %d\n", pXuids[ i ], pfPrivateSlots[ i ] );
    }

    InitializeOverlapped();

    ret = XSessionJoinRemote( m_hSession,
                              dwXuidCount,
                              pXuids,
                              pfPrivateSlots,
                              &m_XOverlapped );

    if( ret != ERROR_IO_PENDING )
    {
        ATG::FatalError( "XSessionJoinRemote failed with error %d\n", ret );
    }

    // Wait for the overlapped operation to complete
    while ( IsProcessingOverlappedOperation() );

    // Make sure we were able to remote local users to the
    // session and then update slot counts
    ret = XGetOverlappedExtendedError( &m_XOverlapped );
    if( ret != ERROR_SUCCESS )
    {
        ATG::FatalError( "Failed to remote local users to session, error 0x%08x\n", HRESULT_FROM_WIN32( ret ) );
    }
    else
    {
        for ( DWORD i=0; i<dwXuidCount; i++ )
        {
            m_Slots[ SLOTS_FILLEDPRIVATE ] += ( pfPrivateSlots[ i ] ) ? 1 : 0;
            m_Slots[ SLOTS_FILLEDPUBLIC ] += ( pfPrivateSlots[ i ] ) ? 0 : 1;
        }

        if( m_Slots[ SLOTS_FILLEDPRIVATE ] > m_Slots[ SLOTS_TOTALPRIVATE ] )
        {
            m_Slots[ SLOTS_FILLEDPRIVATE ] = m_Slots[ SLOTS_TOTALPRIVATE ];

            DWORD diff = m_Slots[ SLOTS_FILLEDPRIVATE ] - m_Slots[ SLOTS_TOTALPRIVATE ];
            m_Slots[ SLOTS_FILLEDPUBLIC ] += diff;

            assert( m_Slots[ SLOTS_FILLEDPUBLIC ] <= m_Slots[ SLOTS_TOTALPUBLIC ] );
            if( m_Slots[ SLOTS_FILLEDPUBLIC ] > m_Slots[ SLOTS_TOTALPUBLIC ] )
            {
                ATG::FatalError( "Too many slots filled!\n" );
            }
        }
    }

    // Free memory associated with pMsg
    delete pMsg;

    DebugDumpSessionDetails();

    m_csSession.Unlock();

    return ret;
}

//------------------------------------------------------------------------
// Name: ProcessRemoveLocalPlayerMessage()
// Desc: Remove local players using the XSessionLeaveLocal API
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ProcessRemoveLocalPlayerMessage( VOID* pMessage )
{
    m_csSession.Lock();

    DWORD ret = ERROR_SUCCESS;

    // Do nothing if the session is already deleted
    if( m_SessionState == ATG::SessionStateDeleted )
    {
        return ret;
    }

    RemoveLocalPlayersMsg* pMsg     = ( RemoveLocalPlayersMsg* )pMessage;
    DWORD dwUserCount               = pMsg->dwUserCount;
    const DWORD* pdwUserIndices     = pMsg->pdwUserIndices;
    const BOOL* pfPrivateSlots      = pMsg->pfPrivateSlots;

    InitializeOverlapped();

    ret = XSessionLeaveLocal( m_hSession,
                              dwUserCount,
                              pdwUserIndices,
                              &m_XOverlapped );

    if( ret != ERROR_IO_PENDING )
    {
        ATG::FatalError( "XSessionLeaveLocal failed with error %d\n", ret );
    }

    // Wait for the overlapped operation to complete
    while ( IsProcessingOverlappedOperation() );

    // Make sure we were able to remove local users from the
    // session and then update slot counts
    ret = XGetOverlappedExtendedError( &m_XOverlapped );
    if( ret != ERROR_SUCCESS )
    {
        ATG::FatalError( "Failed to remove local users from session, error 0x%08x\n", HRESULT_FROM_WIN32( ret ) );
    }
    else
    {
        for ( DWORD i=0; i<dwUserCount; i++ )
        {
            m_Slots[ SLOTS_FILLEDPRIVATE ] -= ( pfPrivateSlots[ i ] ) ? 1 : 0;
            m_Slots[ SLOTS_FILLEDPUBLIC ] -= ( pfPrivateSlots[ i ] ) ? 0 : 1;

            m_Slots[ SLOTS_FILLEDPRIVATE ] = max( m_Slots[ SLOTS_FILLEDPRIVATE ], 0 );
            m_Slots[ SLOTS_FILLEDPUBLIC ] = max( m_Slots[ SLOTS_FILLEDPUBLIC ], 0 );
        }
    }

    // Free memory associated with pMsg
    delete pMsg;

    DebugDumpSessionDetails();

    m_csSession.Unlock();

    return ret;
}

//------------------------------------------------------------------------
// Name: ProcessRemoveRemotePlayerMessage()
// Desc: Add local players using the XSessionLeaveRemote API
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ProcessRemoveRemotePlayerMessage( VOID* pMessage )
{
    m_csSession.Lock();
    DWORD ret = ERROR_SUCCESS;

    // Do nothing if the session is already deleted
    if( m_SessionState == ATG::SessionStateDeleted )
    {
        return ret;
    }

    RemoveRemotePlayersMsg* pMsg    = ( RemoveRemotePlayersMsg* )pMessage;
    DWORD dwXuidCount               = pMsg->dwXuidCount;
    const XUID* pXuids              = pMsg->pXuids;
    const BOOL* pfPrivateSlots      = pMsg->pfPrivateSlots;

    InitializeOverlapped();

    ret = XSessionLeaveRemote( m_hSession,
                               dwXuidCount,
                               pXuids,
                               &m_XOverlapped );

    if( ret != ERROR_IO_PENDING )
    {
        ATG::FatalError( "XSessionLeaveRemote failed with error %d\n", ret );
    }

    // Wait for the overlapped operation to complete
    while ( IsProcessingOverlappedOperation() );

    // Make sure we were able to remove local users from the
    // session and then update slot counts
    ret = XGetOverlappedExtendedError( &m_XOverlapped );
    if( ret != ERROR_SUCCESS )
    {
        ATG::FatalError( "Failed to add local users to session, error 0x%08x\n", HRESULT_FROM_WIN32( ret ) );
    }
    else
    {
        for ( DWORD i=0; i<dwXuidCount; i++ )
        {
            m_Slots[ SLOTS_FILLEDPRIVATE ] -= ( pfPrivateSlots[ i ] ) ? 1 : 0;
            m_Slots[ SLOTS_FILLEDPUBLIC ] -= ( pfPrivateSlots[ i ] ) ? 0 : 1;

            m_Slots[ SLOTS_FILLEDPRIVATE ] = max( m_Slots[ SLOTS_FILLEDPRIVATE ], 0 );
            m_Slots[ SLOTS_FILLEDPUBLIC ] = max( m_Slots[ SLOTS_FILLEDPUBLIC ], 0 );
        }
    }

    // Free memory associated with pMsg
    delete pMsg;

    DebugDumpSessionDetails();

    m_csSession.Unlock();

    return ret;
}

//------------------------------------------------------------------------
// Name: ProcessModifySkillMessage()
// Desc: Modifies the TrueSkill(TM) of a session using the 
//       XSessionModifySkill API
// Thread Safety: Required
//------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManager<TaskQueueType, MessageRouterType>::ProcessModifySkillMessage( VOID* pMessage )
{
    m_csSession.Lock();
    DWORD ret = ERROR_SUCCESS;

    ModifySkillMsg* pMsg    = ( ModifySkillMsg* )pMessage;
    DWORD dwXuidCount       = pMsg->dwXuidCount;
    const XUID* pXuids      = pMsg->pXuids;

    InitializeOverlapped();

    ret = XSessionModifySkill( m_hSession,
                               dwXuidCount,
                               pXuids,
                               &m_XOverlapped );

    if( ret != ERROR_IO_PENDING )
    {
        ATG::FatalError( "XSessionLeaveRemote failed with error %d\n", ret );
    }

    // Free memory associated with pMsg
    delete pMsg;

    m_csSession.Unlock();

    return ret;
}

// Force instantiation of SessionManagerMessageCallback with a specific template
static PFNMESSAGECONSUMERCALLBACK g_pfn = 
    SessionManagerMessageCallback<ATG::TaskQueue,ATG::MessageRouter<ATG::TaskQueue>>;

}
