#pragma once

#ifndef CFtpServer_H
#define CFtpServer_H

#define __USE_FILE_OFFSET64
#define CFTPSERVER_TRANSFER_BUFFER_SIZE	(8 * 1024)


#include <ctime>
#include <cctype>
#include <cstdio>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <stdarg.h>
#include <sys/stat.h>

#include <io.h>
#include <direct.h>
#include <process.h>


#include <xtl.h>
#include <xbdm.h>
#include <malloc.h>
#include <winsockx.h>

#include <process.h>
#include <pmcpb.h>
#include <pmcpbsetup.h>


/***************************************
 * THE CLASS: CFtpServer
 ***************************************/
class CFtpServer
{

	/***************************************
	 * PUBLIC
	 ***************************************/
	public:

		/* Constructor */
		CFtpServer(void);
		/* Destructor */
		~CFtpServer(void);


		/****************************************
		 * USER
		 ****************************************/

		/* The Structure which will be allocated for each User. */
		struct UserNode
		{
			int iMaxClient;
			unsigned char ucPriv;
			char *szName;
			char *szPasswd;
			bool bIsEnabled;
			char *szStartDir;
			int iNbClient;
			struct UserNode *PrevUser;
			struct UserNode *NextUser;
		};

		/* Enumerate the different Privilegies a User can get. */
		enum
		{
			READFILE	= 0x1,
			WRITEFILE	= 0x2,
			DELETEFILE	= 0x4,
			LIST		= 0x8,
			CREATEDIR	= 0x10,
			DELETEDIR	= 0x20,
		};

		/* Create a new User. Don't create a user named "anonymous", call AllowAnonymous();.
			Arguments:
				-the User Name.
				-the User Password.
				-the User Start directory.
			Returns:
				-on success: the adress of the New-User's CFtpServer::UserNode structure.
				-on error: NULL.
		*/
		struct CFtpServer::UserNode *AddUser( const char *szName, const char *szPasswd, const char* szStartDir );

		/* Set the Privilegies of a User.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
				-the user's privilegies of the CFtpServer Enum of privilegies,
					separated by the | operator.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool SetUserPriv( struct CFtpServer::UserNode *User, unsigned char ucPriv );

		/* Delete a User, and by the way all the Clients connected to this User.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool DelUser( struct CFtpServer::UserNode * User );

		/* Get the Head of the User List.
			Arguments:
			Returns:
				-the Head of the User List.
		*/
		struct CFtpServer::UserNode *GetUserListHead( void ) {
			return this->UserListHead;
		}

		/* Get the Last User of the List.
			Arguments:
			Returns:
				-the Last User of the List.
		*/
		struct CFtpServer::UserNode *GetUserListLast( void ) {
			return this->UserListLast;
		}

		/* Get the Next User of the List.
			Arguments:
				-a pointer to a CFtpServer::UserNode structure.
			Returns:
				-the Next User of the list.
		*/
		struct CFtpServer::UserNode *GetNextUser( const struct CFtpServer::UserNode *User ) {
			if( User )
				return User->NextUser;
			return NULL;
		}

		/* Get the Previous User of the List.
			Arguments:
				-a pointer to a CFtpServer::UserNode structure.
			Returns:
				-the Previous User of the List.
		*/
		struct CFtpServer::UserNode *GetPreviousUser( const struct CFtpServer::UserNode *User ) {
			if( User )
				return User->PrevUser;
			return NULL;
		}

		enum {
			Error = -2,
			None = -1,
			Unlimited = 0
		};
		/* Set the maximum number of Clients which can be connected to the User at a time.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
				-the number of clients who will be able to be connected to the user at a time.
					CFtpServer::UserMaxClient.Unlimited: Unlimited.
					CFtpServer::UserMaxClient.None: None.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool SetUserMaxClient( struct CFtpServer::UserNode *User, int iMaxClient );

		/* Get the maximum number of Clients which can be connected to the User at a time.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: the number of clients which can be connected to the User at a time.
				-on error: -2.
		*/
		unsigned int GetUserMaxClient( const struct CFtpServer::UserNode *User ) {
			if( User )
				return User->iMaxClient;
			return 0;
		}

		/* Delete all the Users, and by the way all the Server's Clients connected to a User. */
		void DelAllUser( void );

		/* Get a User's privilegies 
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: the user's privilegies concatenated with the bitwise inclusive
					binary operator "|".
				-on error: 0.
		*/
		unsigned char GetUserPriv( const struct CFtpServer::UserNode *User ) {
			if( User ) {
				return User->ucPriv;
			} else return 0;
		}

		/* Get a pointer to a User's Name.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: a pointer to the User's Name.
				-on error: NULL.
		*/
		const char* GetUserLogin( const struct CFtpServer::UserNode *User ) {
			if( User ) {
				return User->szName;
			} else return NULL;
		}

		/* Get a pointer to a User's Password.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: a pointer to the User's Password.
				-on error: NULL.
		*/
		const char* GetUserPasswd( const struct CFtpServer::UserNode *User ) {
			if( User ) {
				return User->szPasswd;
			} else return NULL;
		}

		/* Get a pointer to a User's Start Directory.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: a pointer to the User's Start Directory.
				-on error: NULL.
		*/
		const char* GetUserStartDir( const struct CFtpServer::UserNode *User ) {
			if( User ) {
				return User->szStartDir;
			} else return NULL;
		}

		/***************************************
		 * START / STOP
		 ***************************************/

		/* Ask the Server to Start Listening on the TCP-Port supplied by SetPort().
			Arguments:
				-the Network Adress CFtpServer will listen on.
					Example:	INADDR_ANY for all local interfaces.
								inet_addr( "127.0.0.1" ) for the TCP Loopback interface.
				-the TCP-Port on which CFtpServer will listen.
			Returns:
				-on success: true.
				-on error: false, the supplied Adress or TCP-Port may not be valid.
		*/
		bool StartListening( unsigned long ulAddr, unsigned short int usPort );

		/* Ask the Server to Stop Listening.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool StopListening( void );

		/* Check if the Server is currently Listening.
			Returns:
				-true: if the Server is currently listening.
				-false: if the Server isn't currently listening.
		*/
		bool IsListening( void ) {
			return this->bIsListening;
		}

		/* Ask the Server to Start Accepting Clients.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool StartAccepting( void );

		/* Check if the Server is currently Accpeting Clients.
			Returns:
				-true: if the Server is currently accepting clients.
				-false: if the Server isn't currently accepting clients.
		*/
		bool IsAccepting( void ) {
			return this->bIsAccepting;
		}

		/****************************************
		 * CONFIG
		 ****************************************/

		/* Get the TCP Port on which CFtpServer will listen for incoming clients.
			Arguments:
			Returns:
				-on success: the TCP-Port.
				-on error: 0.
		*/
		unsigned short GetListeningPort( void ) {
			return this->usPort;
		}

		/* Set the TCP Port Range CFtpServer can use to Send and Receive Files or Data.
			Arguments:
				-the First Port of the Range.
				-the Number of Ports, including the First previously given.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool SetDataPortRange( unsigned short int usDataPortStart, unsigned int iNumber );

		/* Get the TCP Port Range CFtpServer can use to Send and Receive Files or Data.
			Arguments:
				-a Pointer to the First Port.
				-a Pointer to the Number of Ports, including the First.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool GetDataPortRange( unsigned short int *usDataPortStart, int *iNumber ) {
			if( usDataPortStart && iNumber ) {
				*usDataPortStart = this->DataPortRange.usStart;
				*iNumber = this->DataPortRange.iNumber;
				return true;
			}
			return false;
		}

		/* Allow or disallow Anonymous users. Its privilegies will be set to
			CFtpServer::READFILE | CFtpServer::LIST.
			Arguments:
				-true if you want CFtpServer to accept anonymous clients, otherwise false.
				-the Anonymous User Start Directory.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool AllowAnonymous( bool bDoAllow, const char *szStartPath );

		/* Check if Anonymous Users are allowed.
			Returns:
				-true: if Anonymous Users are allowed.
				-false: if Anonymous Users aren't allowed.
		*/
		bool IsAnonymousAllowed( void ) {
			return this->bAllowAnonymous;
		}


		/****************************************
		 * STATISTICS
		 ****************************************/

		/* Get in real-time the number of Clients connected to the Server.
			Returns:
				-the current number of Clients connected to the Server.
		*/
		int GetNbClient( void ) {
			return this->iNbClient;
		}

		/* Get in real-time the number of existing Users.
			Returns:
				-the current number of existing Users.
		*/
		int GetNbUser( void ) {
			return this->iNbUser;
		}

		/* Get the number of Clients currently connected using the specified User.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: the count of Clients using currently the User.
				-on error: -1.
		*/
		int GetNbClientUsingUser( struct CFtpServer::UserNode *User ) {
			if( User ) {
				return User->iNbClient;
			} else return -1;
		}

	/***************************************
	 * PRIVATE
	 ***************************************/
	private:

		/****************************************
		 * CLIENT
		 ****************************************/

		/* Enum the differents Modes of Connection for transfering Data. */
		enum CFtpDataMode {
			Mode_None,
			Mode_PASV,
			Mode_PORT
		};

		/* Enum the differents Status a Client can got. */
		enum CFtpClientStatus {
			Status_Waiting = 0,
			Status_Listing,
			Status_Uploading,
			Status_Downloading
		};

		/* The Structure which will be allocated for each Client. */
		struct ClientNode
		{

			/*****************************************
			 * TRANSFER
			 *****************************************/

			SOCKET DataSock;

			unsigned long ulServerIP;
			unsigned long ulClientIP;

			bool bBinaryMode;

			struct
			{
				__int64 RestartAt;
				struct ClientNode *Client;
				char szPath[ MAX_PATH + 1 ];

				HANDLE hTransferThread;
				unsigned uTransferThreadId;
			} CurrentTransfer;

			/*****************************************
			 * USER
			 *****************************************/

			struct UserNode *User;

			/*****************************************
			 * SHELL
			 *****************************************/

			SOCKET CtrlSock;

			bool bIsLogged;
			bool bIsAnonymous;

			CFtpDataMode eDataMode;
			CFtpClientStatus eStatus;

			unsigned long ulDataIp;
			unsigned short usDataPort;
			
			char* pszRenameFromPath;

			char szCurrentDir[ MAX_PATH + 4 ];

			HANDLE hClientThread;
			unsigned dClientThreadId;

			/******************************************
			 * LINKED LIST
			 ******************************************/

			struct ClientNode *PrevClient;
			struct ClientNode *NextClient;

			/******************************************
			 * OTHER
			 ******************************************/

			bool IsCtrlCanalOpen;
			class CFtpServer *CFtpServerEx;

		};

		/* Add a new Client.
			Returns:
				-on success: a pointer to the new Client's CFtpServer::ClientNode structure.
				-on error: NULL.
		*/
		struct CFtpServer::ClientNode *AddClient( CFtpServer *CFtpServerEx, SOCKET Sock, struct sockaddr_in *Sin );

		/* Delete a Client.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool DelClientEx( struct CFtpServer::ClientNode *Client );

		/* Delete all Clients. */
		void DelAllClient( void );

		/* Parse the Client's Command.
		/	Returns:
				-nothing important.
		*/
		static unsigned __stdcall ClientShell( void *pvParam );

		CRITICAL_SECTION m_csClientRes;

		struct CFtpServer::ClientNode *ClientListHead;
		struct CFtpServer::ClientNode *ClientListLast;

		/*****************************************
		 * Network
		 *****************************************/

		SOCKET ListeningSock;

		struct
		{
			int iNumber;
			unsigned short int usStart;

		} DataPortRange;


		/* Send a reply to a Client.
			Returns:
				-on success: true.
				-on error: false; The Socket may be invalid;
					The Connection may have been interupted.
		*/
		bool SendReply( struct CFtpServer::ClientNode *Client, const char *pszReply );
		bool SendReply2( struct CFtpServer::ClientNode *Client, const char *pszList, ... );
		
		/* Close a Socket. */
		int CloseSocket( SOCKET Sock ) 
		{
			return closesocket( Sock );
		}

		/*****************************************
		 * FILE
		 *****************************************/

		char *szMonth[12];

		/* Simplify a Path. */
		bool SimplifyPath( char *pszPath );

		bool SimplifyPathXDK( char *pszPath,char *pszPath1 );
		bool filePathXDK( char *pszPath,char *pszPath1 );

		/* Build a Path using the Client Command and the Client's User Start-Path. */
		char* BuildPath( const char* szStartPath, const char* szCurrentPath, char* szAskedPath );

		/* Structure used by the layer of Abstraction in order to list a Directory
			on both Linux and Win32 OS.
		*/
		struct LIST_FindFileInfo
		{
			long hFile;
			char* pszTempPath;
			struct _finddatai64_t c_file;

			char *pszName;

			struct
			{
				bool Read;
				bool Write;
				bool Execute;
			} OwnerPermission, GroupPermission, OtherPermission;

			bool IsDirectory;

			struct tm tm_write;
			time_t time_t_write;
			char szDirPath[ MAX_PATH + 1 ];
			char szFullPath[ MAX_PATH + 1 ];
			__int64 Size;
		};

		/* Layer of Abstraction which list a Directory on both Linux and Win32 */
		struct LIST_FindFileInfo *LIST_FindFirstFile( char *szPath );
		void LIST_FindClose( struct LIST_FindFileInfo *fi );
		bool LIST_FindNextFile( struct LIST_FindFileInfo *fi );

		/* Process the LIST, NLST Commands.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool LIST_Command( struct CFtpServer::ClientNode *Client, char *pszCmdArg, const char *pszCmd );

		/* Process the STOR, STOU, APPE Commands.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool STOR_Command( struct CFtpServer::ClientNode *Client, char *pszCmdArg, const char *pszCmd );

		/*****************************************
		 * TRANSFER
		 *****************************************/

		/* Open the Data Canal in order to transmit data.
			Returns:
			-on success: true.
			-on error: false.
		*/
		bool OpenDataConnection(struct CFtpServer::ClientNode *Client);

		/* Open the Data Canal in order to transmit data */
		static unsigned __stdcall RetrieveThread( void *pvParam );
		static unsigned __stdcall StoreThread( void *pvParam  );

		/*****************************************
		 * USER
		 *****************************************/

		CRITICAL_SECTION m_csUserRes;

		struct UserNode *AnonymousUser;

		struct UserNode *UserListHead;

		struct UserNode *UserListLast;

		struct UserNode *AddUserEx( const char *szName, const char *szPasswd, const char* szStartDir );

		struct UserNode * SearchUserFromName( const char *szName );

		/*****************************************
		 * CONFIG
		 *****************************************/

		int iReplyMaxLen;

		unsigned int iCheckPassDelay;

		unsigned short int usPort;

		bool bAllowAnonymous;

		int iNbClient;
		int iNbUser;

		bool bIsListening;
		bool bIsAccepting;

		HANDLE hAcceptingThread;
		unsigned uAcceptingThreadID;
		static unsigned __stdcall CFtpServer::StartAcceptingEx( void *pvParam );

};

#endif