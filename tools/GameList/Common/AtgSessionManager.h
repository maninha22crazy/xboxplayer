//-----------------------------------------------------------------------------
// AtgSessionManager.h
//
// Class for managing all session-related activity for a single
// XSession instance
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#include "AtgMessages.h"
#include "AtgUtil.h"
#include "AtgTasks.h"
#include "AtgSyncTaskQueue.h"
#include "AtgMessageRouter.h"

namespace ATG
{

//--------------------------------------------------------------------------------------
// Callback for MessageRouter
//--------------------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
extern VOID SessionManagerMessageCallback( DWORD dwMessageId, VOID* pMsg, VOID* pConsumer );

//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------
const DWORD MODIFY_FLAGS_ALLOWED = XSESSION_CREATE_USES_ARBITRATION |
                                   XSESSION_CREATE_INVITES_DISABLED |
                                   XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED |
                                   XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED;

const XOVERLAPPED_TIMEOUT        = 5; // Timeout for blocking on m_hTaskCompletedEvent

//------------------------------------------------------------------------
// Name: enum SessionState
// Desc: Session state enumeration       
//------------------------------------------------------------------------
enum SessionState
{
    SessionStateNone,
    SessionStateCreate,                 // XSessionCreate task created
    SessionStateCreating,               // XSessionCreate has been called
    SessionStateCreated,                // XSessionCreate has returned
    SessionStateIdle,                   // Session waiting for prompts
    SessionStateWaitingForRegistration, // Session waiting to register with arbitration
    SessionStateRegister,               // XSessionArbitrationRegister task created
    SessionStateRegistering,            // XSessionArbitrationRegister has been called
    SessionStateRegistered,             // XSessionArbitrationRegiste has returned
    SessionStateStart,                  // XSessionStart task created
    SessionStateStarting,               // XSessionStart has been called
    SessionStateInGame,                 // XSessionStart has returned
    SessionStateMigrateHost,            // XSessionMigrateHost has been called
    SessionStateEnd,                    // XSessionEnd task created
    SessionStateEnding,                 // XSessionEnd has been called
    SessionStateEnded,                  // XSessionEnd has returned
    SessionStateDelete,                 // XSessionDelete task created
    SessionStateDeleting,               // XSessionDelete has been called, session is invalid
    SessionStateDeleted                 // XSessionDelete has returned, session is invalid
};

//------------------------------------------------------------------------
// Name: enum SessionState
// Desc: Slot types for the session       
//------------------------------------------------------------------------
typedef enum _SLOTS
{
    SLOTS_TOTALPUBLIC,
    SLOTS_TOTALPRIVATE,
    SLOTS_FILLEDPUBLIC,
    SLOTS_FILLEDPRIVATE,
    SLOTS_MAX
} SLOTS;

//------------------------------------------------------------------------
// Name: struct SessionManagerInitParams
// Desc: Initialization parameters       
//------------------------------------------------------------------------
template <class TaskQueueType>
struct SessionManagerInitParams
{
    DWORD                           m_dwSessionFlags;
    BOOL                            m_bIsHost;
    UINT                            m_dwMaxPublicSlots;
    UINT                            m_dwMaxPrivateSlots;
    MessageRouter<TaskQueueType>*   m_pMessageRouter;
};

//--------------------------------------------------------------------------------------
// Name: class SessionManager
// Desc: Multithreaded class for managing all XSession-related calls for a single 
//       LIVE session. Thread safety is enforced by an internal critical section 
//       object, so no extra work for ensuring thread safety is required by
//       users of a class instance.
//--------------------------------------------------------------------------------------
template <class TaskQueueType, class MessageRouterType>
DWORD SessionManagerTaskExecutionThreadProc( VOID* lpThreadParameter );

template <class TaskQueueType, class MessageRouterType>
class SessionManager
{
#define NUM_SESSION_TASK_THREADS 0          // set to 0 for single-threaded SessionManager
                                            // non-0 for multi-threaded
#define SESSIONMANAGER_SIGNATURE 0xF00DF00D

friend DWORD SessionManagerTaskExecutionThreadProc
    <TaskQueueType, MessageRouterType>( VOID* lpThreadParameter );

friend VOID SessionManagerMessageCallback
    <TaskQueueType, MessageRouterType>( DWORD dwMessageId,
                                        VOID* pMsg,
                                        VOID* pConsumer );

public:

    SessionManager( VOID );
    virtual ~SessionManager( VOID );

    BOOL Initialize( ATG::SessionManagerInitParams<TaskQueueType> initParams );
    BOOL IsInitialized( VOID );

    DWORD GetSessionOwner( VOID );
    XUID GetSessionOwnerXuid( VOID );
    VOID SetSessionOwner( DWORD dwOwnerController );
    BOOL IsSessionOwner( DWORD dwController );
    BOOL IsSessionOwner( XUID xuidOwner );

    BOOL IsSessionHost( VOID );
    VOID MakeSessionHost( VOID );

    ULONGLONG GetSessionNonce( VOID );
    VOID SetTrackingNonce( ULONGLONG qwNonce );

    VOID SetHostInAddr( const IN_ADDR& inaddr );
    IN_ADDR GetHostInAddr( VOID ) const;

    XNKID GetSessionID( VOID );

    BOOL HasSessionFlags( DWORD dwFlags );
    DWORD GetSessionFlags( VOID );

    VOID GetMaxSlotCounts( DWORD& dwMaxPublicSlots, DWORD& dwMaxPrivateSlots );
    VOID GetFilledSlotCounts( DWORD& dwFilledPublicSlots, DWORD& dwFilledPrivateSlots );

    VOID SetMaxSlotCounts( DWORD dwMaxPublicSlots, DWORD dwMaxPrivateSlots );
    VOID SetFilledSlotCounts( DWORD dwFilledPublicSlots, DWORD dwFilledPrivateSlots );

    const XSESSION_INFO& GetSessionInfo( VOID ) const;
    VOID SetSessionInfo( const XSESSION_INFO& session_info );
    VOID SetNewSessionInfo( const XSESSION_INFO& session_info );

    VOID SetSessionFlags( DWORD dwFlags, BOOL fClearExisting = FALSE );
    VOID FlipSessionFlags( DWORD dwFlags );
    VOID ClearSessionFlags( DWORD dwFlags );
    DWORD ModifySessionFlags( DWORD dwFlags );

    WCHAR* GetSessionError( VOID );
    VOID SetSessionError( WCHAR* error );

    SessionState GetSessionState( VOID );
    VOID SwitchToState( SessionState newState );

    const PXSESSION_REGISTRATION_RESULTS GetRegistrationResults();

    HRESULT ModifySkill( const DWORD dwNumXuids, const XUID* pXuids );

    HRESULT AddLocalPlayers( const DWORD dwUserCount,
                                 const DWORD* pdwUserIndices,
                                 const BOOL* pfPrivateSlots );
    HRESULT RemoveLocalPlayers( const DWORD dwUserCount,
                                      const DWORD* pdwUserIndices,
                                      const BOOL* pfPrivateSlots );
    HRESULT AddRemotePlayers( const DWORD dwXuidCount,
                                    const XUID* pXuids,
                                    const BOOL* pfPrivateSlots );
    HRESULT RemoveRemotePlayers( const DWORD dwXuidCount,
                                         const XUID* pXuids,
                                         const BOOL* pfPrivateSlots );

    HRESULT WriteStats( XUID xuid,
                          DWORD dwNumViews,
                          const XSESSION_VIEW_PROPERTIES *pViews );

    VOID StartQoSListener( BYTE* data, UINT dataLen, DWORD bitsPerSec );

    BOOL IsProcessingOverlappedOperation( VOID ) const;
    DWORD GetOverlappedExtendedError( VOID ) const;

    DWORD GetSignature( VOID ) const { return m_dwSig; } 
    BOOL ValidityCheck( VOID ) const;

    VOID DebugDumpSessionDetails( VOID );

protected:
    VOID Reset( VOID );

    HRESULT CreateSession( VOID );
    HRESULT RegisterArbitration( VOID );
    HRESULT StartSession( VOID );
    HRESULT EndSession( VOID );
    HRESULT MigrateSession( VOID );
    HRESULT DeleteSession( VOID );

    DWORD ProcessCreateSessionMessage( VOID* pMessage );
    DWORD ProcessStartSessionMessage( VOID* pMessage );
    DWORD ProcessModifySessionFlagsMessage( VOID* pMessage );
    DWORD ProcessModifySkillMessage( VOID* pMessage );
    DWORD ProcessWriteStatsMessage( VOID* pMessage );
    DWORD ProcessEndSessionMessage( VOID* pMessage );
    DWORD ProcessMigrateHostMessage( VOID* pMessage );
    DWORD ProcessDeleteSessionMessage( VOID* pMessage );
    DWORD ProcessArbitrationRegisterMessage( VOID* pMessage );
    DWORD ProcessAddLocalPlayerMessage( VOID* pMessage );
    DWORD ProcessAddRemotePlayerMessage( VOID* pMessage );
    DWORD ProcessRemoveLocalPlayerMessage( VOID* pMessage );
    DWORD ProcessRemoveRemotePlayerMessage( VOID* pMessage );

    VOID SetSessionState( SessionState newState );
    BOOL CheckSessionState( SessionState expectedState );

    VOID StopQoSListener();
    
private:
    SessionManager& operator=( const SessionManager& ref ) {}
    VOID InitializeOverlapped();

protected:
    const DWORD             m_dwSig;                    // Used for checking that a memory location
                                                        // holds a valid SessionManager instance
    BOOL                    m_bIsInitialized;           // Initialized?
    BOOL                    m_bIsHost;                  // Is hosting
    BOOL                    m_bUsingQoS;                // Is the QoS listener enabled
    DWORD                   m_dwOwnerController;        // Which controller created the session
    XUID                    m_xuidOwner;                // XUID of gamer who created the session
    WCHAR*                  m_strSessionError;          // Error message for current session
    MessageRouterType*      m_pMessageRouter;           // MessageRouter for current session
    HANDLE*                 m_phTaskThreads;
    XOVERLAPPED             m_XOverlapped;              // Overlapped structure for the session
    HANDLE                  m_hTaskCompletedEvent;      // Signallable event for overlapped calls
    ULONGLONG               m_qwSessionNonce;           // Session nonce
    ULONGLONG               m_qwTrackingNonce;          // Tracking nonce
    XSESSION_INFO           m_SessionInfo;              // Session ID, key, and host address 
    XSESSION_INFO           m_NewSessionInfo;           // New session ID, key, and host address 
    HANDLE                  m_hSession;                 // Session handle
    SessionState            m_SessionState;             // Current session state
    DWORD                   m_dwSessionFlags;           // Flags for the current session    
    ATG::CritSec            m_csSession;                // Critical section object to manage
                                                        // synchronization of calls over multiple
                                                        // threads that modify session state
    const BOOL              m_bIsMultithreaded;         // Is task processing done on background
                                                        // threads?
    BOOL                    m_bShutdownBGThreads;       // Should task processing threads exit?
    UINT                    m_Slots[ SLOTS_MAX ];       // Filled/open slots for the session
    IN_ADDR                 m_HostInAddr;               // Host IP address
    
    PXSESSION_REGISTRATION_RESULTS m_pRegistrationResults;
};

}
