//-----------------------------------------------------------------------------
// AtgMessages.h
//
// Defines basic data structures using for passing messages. Used with the
// AtgAsyncTaskQueue class
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#include "xbox.h"

namespace ATG
{

//
// Message Identifiers
// The high word of a message identifier gives the message area
// The low word of a message identifier gives the message number
//

#define MSGID(Area, Number)         (DWORD)((WORD)(Area) << 16 | (WORD)(Number))
#define MSG_AREA(msgid)             (((msgid) >> 16) & 0xFFFF)
#define MSG_NUMBER(msgid)           ((msgid) & 0xFFFF)

//
// Message Areas
//

#define MSGAREA_NONE                (0x0000)
#define MSGAREA_SESSIONS            (0x0001)
#define MSGAREA_APP                 (0x0002)

//
// Session-related Messages
//
// IMPORTANT: The actual values of the messages are not important,
//            but the relative order is. When adding, deleting, or
//            changing messages, please make sure that the
//            relative order is in the order that these messages
//            would be processed if they were added to a priority
//            queue. The lower the id, the higher the priority.
//

#define MSG_SESSION_CREATE                  MSGID(MSGAREA_SESSIONS, 0x0001)
#define MSG_SESSION_ADDLOCAL                MSGID(MSGAREA_SESSIONS, 0x0002)
#define MSG_SESSION_ADDREMOTE               MSGID(MSGAREA_SESSIONS, 0x0003)
#define MSG_SESSION_REMOVELOCAL             MSGID(MSGAREA_SESSIONS, 0x0004)
#define MSG_SESSION_REMOVEREMOTE            MSGID(MSGAREA_SESSIONS, 0x0005)
#define MSG_SESSION_START                   MSGID(MSGAREA_SESSIONS, 0x0006)
#define MSG_SESSION_MODIFYFLAGS             MSGID(MSGAREA_SESSIONS, 0x0007)
#define MSG_SESSION_MODIFYSKILL             MSGID(MSGAREA_SESSIONS, 0x0008)
#define MSG_SESSION_REGISTERARBITRATION     MSGID(MSGAREA_SESSIONS, 0x0009)
#define MSG_SESSION_WRITESTATS              MSGID(MSGAREA_SESSIONS, 0x000A)
#define MSG_SESSION_END                     MSGID(MSGAREA_SESSIONS, 0x000B)
#define MSG_SESSION_MIGRATE                 MSGID(MSGAREA_SESSIONS, 0x000C)
#define MSG_SESSION_DELETE                  MSGID(MSGAREA_SESSIONS, 0x000D)

//-----------------------------------------------------------------------------
//  Session API message structures
//-----------------------------------------------------------------------------

struct SessionCreateMsg
{
    DWORD           dwFlags;
    DWORD           dwMaxPublicSlots;
    DWORD           dwMaxPrivateSlots;
    DWORD           dwUserIndex;
};

struct SessionStartMsg
{
};

struct SessionEndMsg
{
};

struct SessionDeleteMsg
{
};

struct SessionModifyFlagsMsg
{
    DWORD           dwFlags;
};

struct ArbitrationRegisterMsg
{
};

struct SessionMigrateMsg
{
};

struct AddLocalPlayersMsg
{
    DWORD                           dwUserCount;
    const DWORD*                    pdwUserIndices;
    const BOOL*                     pfPrivateSlots;

    AddLocalPlayersMsg()
    {
        pdwUserIndices = NULL;
        pfPrivateSlots = NULL;
    }

    ~AddLocalPlayersMsg()
    {
        if( pdwUserIndices )
        {
            free( (VOID*)pdwUserIndices );
        }

        if( pfPrivateSlots )
        {
            free( (VOID*)pfPrivateSlots );
        }
    }

};

struct AddRemotePlayersMsg
{
    DWORD                           dwXuidCount;
    const XUID*                     pXuids;
    const BOOL*                     pfPrivateSlots;

    AddRemotePlayersMsg()
    {
        pXuids         = NULL;
        pfPrivateSlots = NULL;
    }

    ~AddRemotePlayersMsg()
    {
        if( pXuids )
        {
            free( (VOID*)pXuids );
        }
        
        if( pfPrivateSlots )
        {
            free( (VOID*)pfPrivateSlots );
        }
    }

};

struct RemoveLocalPlayersMsg
{
    DWORD                           dwUserCount;
    const DWORD*                    pdwUserIndices;
    const BOOL*                     pfPrivateSlots;

    RemoveLocalPlayersMsg()
    {
        pdwUserIndices = NULL;
        pfPrivateSlots = NULL;
    }

    ~RemoveLocalPlayersMsg()
    {
        if( pdwUserIndices )
        {
            free( (VOID*)pdwUserIndices );
        }

        if( pfPrivateSlots )
        {
            free( (VOID*)pfPrivateSlots );
        }
    }

};

struct RemoveRemotePlayersMsg
{
    DWORD                           dwXuidCount;
    const XUID*                     pXuids;
    const BOOL*                     pfPrivateSlots;

    RemoveRemotePlayersMsg()
    {
        pXuids         = NULL;
        pfPrivateSlots = NULL;
    }

    ~RemoveRemotePlayersMsg()
    {
        if( pXuids )
        {
            free( (VOID*)pXuids );
        }
        
        if( pfPrivateSlots )
        {
            free( (VOID*)pfPrivateSlots );
        }
    }

};

struct ModifySkillMsg
{
    DWORD                           dwXuidCount;
    const XUID*                     pXuids;

    ModifySkillMsg()
    {
        pXuids = NULL;
    }

    ~ModifySkillMsg()
    {
        if( pXuids )
        {
            free( (VOID*)pXuids );
        }
    }

};

struct WriteStatsMsg
{
    XUID                            xuid;
    DWORD                           dwNumViews;
    const XSESSION_VIEW_PROPERTIES* pViews;

    WriteStatsMsg()
    {
        pViews = NULL;
    }

    ~WriteStatsMsg()
    {
        if( pViews )
        {
            free( (VOID*)pViews );
        }
    }

};

} // namespace ATG
