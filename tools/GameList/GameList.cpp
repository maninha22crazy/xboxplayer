/*=====================================================================//
//	Name:		GameList.cpp
//	Desc:		程序入口，包括UI界面关联设置
//	Coder:		GooHome、EME
//	Date:		2009-12-23
//=====================================================================*/

#include <xtl.h>
#include <xui.h>
#include <xuiapp.h>
#include <AtgUtil.h>

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include "Utility.h"
#include "GameList.h"
#include "..\DeviceMgrLib\DeviceMgrLib.h"
#include <algorithm>
#include "AtgXmlFileParser.h"
#include "AtgXmlWriter.h"
#include "CFtpServer.h"

#include <xam.h>
#include "xfilecache.h"
#include "smc.h"

#include "CFtpClient.h"
#include <vector>

// 下载线程
static DWORD WINAPI DownFileThread(LPVOID pAram);

SOCKET m_sockDateDownT;			// 数据传送socket(下载)
SOCKET m_sockClientDownT;		    // 命令socket(下载)
CFtpClient m_ftpClient;                  //  下载操作


typedef std::vector <DownNode> DownList;
DownList m_DownList;

struct xex_rec 
{
	int var1;
	int var2;
};

struct header_1
{
	int mediaid;
	int version1;
	int version2;
	int titleid;
	char unused;
	char unused2;
	char diskno;
	char diskcount;
};

#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
typedef struct _STRING 
{        
	USHORT Length;        
	USHORT MaximumLength;       
	PCHAR Buffer;    
} STRING;

#ifdef __cplusplus
extern "C" {
#endif

	HRESULT __stdcall ObCreateSymbolicLink( STRING*, STRING*);
	HRESULT __stdcall ObDeleteSymbolicLink( STRING* );
	UINT32 __stdcall XexGetModuleHandle(char* module, PVOID hand);
	UINT32 __stdcall XexGetProcedureAddress(UINT32 hand ,UINT32, PVOID);

#ifdef __cplusplus
}
#endif

void (*XamLoaderLaunchTitle)(const char *, UINT32);

#define XAMCONTENTOPENFILE_ORD (UINT32)0x269
UINT32 (*XamContentOpenFile)(UINT32, char*, char*,UINT32,UINT32,UINT32,UINT32*);

#define XAMCONTENTCLOSE_ORD (UINT32)0x25a
UINT32 (*XamContentClose)(char*,UINT32*);



using namespace std; 

CXuiImageElement m_WallImage;
CXuiList m_List;
smc MeinSMC;
float * temperaturen;
int FSpeed = 60;


//--------------------------------------------------------------------------------------
// Name: debugLog
// Desc: 输出日志信息到debug.log
//--------------------------------------------------------------------------------------
void debugLog(char* output)
{
	ofstream writeLog;
	writeLog.open("game:\\debug.log",ofstream::app);
	if (writeLog.is_open())
	{
		writeLog.write(output,strlen(output));
		writeLog.write("\n",1);
	}
	writeLog.close();
}

//--------------------------------------------------------------------------------------
// Name: FileExists
// Desc: 判断文件是否存在
//--------------------------------------------------------------------------------------
bool FileExists(char* strFilename) 
{
	struct stat stFileInfo;
	return stat(strFilename,&stFileInfo) == 0;
}

//--------------------------------------------------------------------------------------
// Name: launchX
// Desc: XLaunchNewImage扩展
//--------------------------------------------------------------------------------------
void launchX(char* xfile)
{
	XFlushUtilityDrive();
	XFileCacheInit(XFILECACHE_CLEAR_ALL,0,XFILECACHE_DEFAULT_THREAD,0,1);
	XFileCacheShutdown();
	XFlushUtilityDrive();
	XLaunchNewImage(xfile,0);
}

//============================================================ xbla begin ==============================================
UINT32 resolveFunct(char* modname, UINT32 ord)
{
	UINT32 ptr32=0, ret=0, ptr2=0;
	ret = XexGetModuleHandle(modname, &ptr32); //xboxkrnl.exe xam.dll?
	if(ret == 0)
	{
		ret = XexGetProcedureAddress(ptr32, ord, &ptr2 );
		if(ptr2 != 0)
			return ptr2;
	}
	return 0;
}

HRESULT Mount(CHAR* szDrive, CHAR* szDevice)
{
	CHAR * szSourceDevice;
	CHAR szDestinationDrive[MAX_PATH];
	USHORT len;
	szSourceDevice = szDevice;

	sprintf_s(szDestinationDrive,MAX_PATH,"\\??\\%s",szDrive);
	len = (USHORT)strlen(szSourceDevice);
	STRING DeviceName =
	{
		len,
		len + 1,
		szSourceDevice
	};
	len = (USHORT)strlen(szDestinationDrive);
	STRING LinkName =
	{
		len,
		len + 1,
		szDestinationDrive
	};
	ObDeleteSymbolicLink( &DeviceName );
	return ( HRESULT )ObCreateSymbolicLink(&LinkName, &DeviceName);
}

UINT32 mountCon(CHAR* szDrive, CHAR* szDevice, CHAR* szPath)
{
	UINT32 ret;
	CHAR szMountPath[MAX_PATH];
	if(XamContentOpenFile == 0)
	{
		ret = resolveFunct("xam.xex", XAMCONTENTOPENFILE_ORD); 
		XamContentOpenFile = (UINT32 (__cdecl *)(UINT32, char*, char*,UINT32,UINT32,UINT32,UINT32*))ret;
		if(XamContentOpenFile == 0)
			return ERROR_INVALID_HANDLE;
	}
	sprintf_s(szMountPath,MAX_PATH,"\\??\\%s\\%s",szDevice,szPath);
	return XamContentOpenFile(0xFE, szDrive, szMountPath, 0x4000003,0,0,0);
}

UINT32 unmountCon(CHAR* szDrive)
{
	USHORT len;
	UINT32 ret;
	CHAR szMountPath[MAX_PATH];
	if(XamContentClose == 0)
	{
		ret = resolveFunct("xam.xex", XAMCONTENTCLOSE_ORD);
		XamContentClose = (UINT32 (__cdecl *)(char *,UINT32 *))ret;
		if(XamContentClose == 0)
			return ERROR_INVALID_HANDLE;
	}
	sprintf_s(szMountPath,MAX_PATH,"\\??\\%s",szDrive);
	return  XamContentClose(szMountPath,0);
}

//============================================================ xbla end ==============================================


#define  COMPARE_LENGTH 20  //比较字符串长度为20字节
//--------------------------------------------------------------------------------------
// Name: lessGameName
// Desc: 对游戏列表进行降序排列
//--------------------------------------------------------------------------------------
bool lessGameName(const GameNode& s1,const GameNode& s2) 
{ 
	return wmemcmp(s1.strName ,s2.strName, COMPARE_LENGTH) > 0; 
}

//--------------------------------------------------------------------------------------
// Name: greaterGameName
// Desc: 对游戏列表进行升序排列
//--------------------------------------------------------------------------------------
bool greaterGameName(const GameNode& s1,const GameNode& s2) 
{ 
	return wmemcmp(s1.strName,s2.strName, COMPARE_LENGTH) < 0; 
} 

//--------------------------------------------------------------------------------------
// Name: lessCreateTime
// Desc: 对游戏列表进行日期降序排列
//--------------------------------------------------------------------------------------
bool lessCreateTime(const GameNode& s1,const GameNode& s2) 
{ 
	return CompareFileTime(&(s1.ftCreationTime), &(s2.ftCreationTime)) == 1; 
}

//--------------------------------------------------------------------------------------
// Name: greaterCreateTime
// Desc: 对游戏列表进行日期升序排列
//--------------------------------------------------------------------------------------
bool greaterCreateTime(const GameNode& s1,const GameNode& s2) 
{ 
	return CompareFileTime(&(s1.ftCreationTime), &(s2.ftCreationTime)) == -1; 
} 

//--------------------------------------------------------------------------------------
// Name: SortList
// Desc: 对游戏列表进行排序
//--------------------------------------------------------------------------------------
VOID SortList(UINT SortType)
{
	if(m_GameList.size() == 0)
	{
		return;
	}
	if(SortType == 0)
	{
		if(m_bSortLess)
		{
			sort(m_GameList.begin(), m_GameList.end(),lessCreateTime);
		}
		else
		{
			sort(m_GameList.begin(), m_GameList.end(),greaterGameName);
		}
	}
	else if(SortType == 1)
	{
		if(m_bSortLess)
		{
			sort(m_GameList.begin(), m_GameList.end(),lessGameName);
		}
		else
		{
			sort(m_GameList.begin(), m_GameList.end(),greaterGameName);
		}
	}
}

//--------------------------------------------------------------------------------------
// Name: LoadConfig
// Desc: 读取配置文件XexDash.xml
//--------------------------------------------------------------------------------------
VOID LoadConfig(VOID)
{
	//ATG::XMLParser parser;
	//ATG::XmlFileParser xmlFile;
	//parser.RegisterSAXCallbackInterface( &xmlFile );
	//HRESULT hr = parser.ParseXMLFile( m_strConfigPath );

	// 解析xml，读取信息
	ATG::XmlFileParser xmlFile;
	HRESULT hr = xmlFile.LoadXMLFile( m_strConfigPath,NULL,0);

	if( SUCCEEDED( hr ) )
	{
	}
}

//--------------------------------------------------------------------------------------
// Name: SaveConfig
// Desc: 保存配置文件XexDash.xml
//--------------------------------------------------------------------------------------
VOID SaveConfig(VOID)
{

	bool ret = CreateDirectory( "Hdd\\XEXDASH", NULL);

	ATG::XMLWriter parser;

	parser.Initialize( m_strConfigPath );

	parser.StartElement("items");
	parser.AddAttribute("description","adapt to Gamelist");

	parser.StartElement("item");
	parser.AddAttribute("name","oemcode");
	parser.AddAttribute("value",m_ConfigNode.nOemCode);
	parser.AddAttribute("description","");
	parser.EndElement();

	parser.StartElement("item");
	parser.AddAttribute("name","language");
	parser.AddAttribute("value",m_ConfigNode.nLanguage);
	parser.AddAttribute("description","");
	parser.EndElement();

	parser.StartElement("item");
	parser.AddAttribute("name","device");
	parser.AddAttribute("value",m_curDevice.deviceName);
	parser.AddAttribute("description","");
	parser.EndElement();

	parser.StartElement("item");
	parser.AddAttribute("name","showwall");
	parser.AddAttribute("value",m_ConfigNode.nShowWall);
	parser.AddAttribute("description","");
	parser.EndElement();

	parser.StartElement("item");
	parser.AddAttribute("name","shownewwall");
	parser.AddAttribute("value",m_ConfigNode.nShowNewWall);
	parser.AddAttribute("description","");
	parser.EndElement();

	parser.StartElement("item");
	parser.AddAttribute("name","wallPath");
	parser.AddAttribute("value",UnicodeToAnsi(m_ConfigNode.strWallPath));			// debug:中文目录保存后，打开无背景的问题 date:2010-01-30 by:EME
	parser.AddAttribute("description","");
	parser.EndElement();

	parser.StartElement("item");
	parser.AddAttribute("name","gametype");
	parser.AddAttribute("value",m_ConfigNode.nGameType);
	parser.AddAttribute("description","");
	parser.EndElement();


	parser.EndElement();
	parser.Close();
}


//--------------------------------------------------------------------------------------
// Name: getGameTitle
// Desc: 游戏文件说明
//--------------------------------------------------------------------------------------
bool getGameTitle(char* lstrFileName,char* lpGameName)
{
	HANDLE hFile = CreateFile(lstrFileName,
		GENERIC_READ,
		FILE_SHARE_READ, 
		NULL, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL );

	DWORD nBytesToRead=40;
	DWORD nBytesRead=40,dwError;
	char lpBuffer[MAX_PATH] = "";
	bool bReturn=false;

	if (hFile != INVALID_HANDLE_VALUE)
	{		
		int bResult = ReadFile(hFile, lpBuffer, nBytesToRead, &nBytesRead,NULL) ; 
		if (bResult>0 && nBytesRead > 0) bReturn=true;
		sprintf(lpGameName, "%s",lpBuffer);
		CloseHandle(hFile);
	}
	return bReturn;
}	

//--------------------------------------------------------------------------------------
// Name: LoadList
// Desc: 加载列表
//--------------------------------------------------------------------------------------
VOID LoadList()
{
	switch(m_ConfigNode.nGameType)
	{
	case 1:
		LoadXblaList();
		break;
	default:
		LoadGameList();
	}
}


//--------------------------------------------------------------------------------------
// Name: LoadXblaList
// Desc: 读取Xbla列表
//
// 000D0000  -arc
// 00080000  -demo
// 00070000  -硬盘版游戏
// 00050000  -1代xbox
// "Content\\0000000000000000\\ID\\type\\XXXXXXXXXXXXXXXXX"
//
//--------------------------------------------------------------------------------------
void LoadXblaList()
{
	// 清空游戏列表
	m_GameList.clear();

   WIN32_FIND_DATA ffd;
   WIN32_FIND_DATA ffd1;
   HANDLE hFind = INVALID_HANDLE_VALUE;
   HANDLE hFind1 = INVALID_HANDLE_VALUE;
   DWORD dwError=0;

   int nFindType = 0;
   struct stat st;

   int nCount = 0;
   char szDir[MAX_PATH];
   char szDir1[MAX_PATH];
   memset(szDir, 0, MAX_PATH);
   memset(szDir1, 0, MAX_PATH);
   sprintf(szDir, "%s\\Content\\0000000000000000\\*", m_curDevice.deviceName);

   hFind = FindFirstFile(szDir, &ffd);
   if (INVALID_HANDLE_VALUE == hFind) 
   {
      return;
   }
   do
   {
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
		  // 目录
		  memset(szDir1, 0, MAX_PATH);
		  sprintf(szDir1, "%s\\Content\\0000000000000000\\%s\\000D0000", m_curDevice.deviceName,ffd.cFileName);
		  nFindType = 0;
		  if( stat( szDir1, &st ) == 0 && S_ISDIR( st.st_mode ) )
		  {
			  memset(szDir1, 0, MAX_PATH);
			  sprintf(szDir1, "%s\\Content\\0000000000000000\\%s\\000D0000\\*", m_curDevice.deviceName,ffd.cFileName);
			  hFind1 = FindFirstFile(szDir1, &ffd1);
			  if (hFind1 != INVALID_HANDLE_VALUE && !(ffd1.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			  {
				memset(szDir1, 0, MAX_PATH);
				sprintf(szDir1, "%s\\Content\\0000000000000000\\%s\\000D0000\\%s", m_curDevice.deviceName,ffd.cFileName,ffd1.cFileName);
				nFindType = 1;
				CloseHandle(hFind1) ;
			  }
		  }
		  else
		  {
			  memset(szDir1, 0, MAX_PATH);
			  sprintf(szDir1, "%s\\Content\\0000000000000000\\%s\\00080000\\*", m_curDevice.deviceName,ffd.cFileName);
			  if( stat( szDir1, &st ) == 0 && S_ISDIR( st.st_mode ) )
			  {
				   hFind1 = FindFirstFile(szDir1, &ffd1);
				  if (hFind1 != INVALID_HANDLE_VALUE && !(ffd1.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				  {
					memset(szDir1, 0, MAX_PATH);
					sprintf(szDir1, "%s\\Content\\0000000000000000\\%s\\00080000\\%s", m_curDevice.deviceName,ffd.cFileName,ffd1.cFileName);
					nFindType = 2;
					CloseHandle(hFind1) ;
				  }
			  }
			  else
			  {
				  memset(szDir1, 0, MAX_PATH);
				  sprintf(szDir1, "%s\\Content\\0000000000000000\\%s\\00070000\\*", m_curDevice.deviceName,ffd.cFileName);
				  if( stat( szDir1, &st ) == 0 && S_ISDIR( st.st_mode ) )
				  {
					   hFind1 = FindFirstFile(szDir1, &ffd1);
					  if (hFind1 != INVALID_HANDLE_VALUE && !(ffd1.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					  {
						memset(szDir1, 0, MAX_PATH);
						sprintf(szDir1, "%s\\Content\\0000000000000000\\%s\\00070000\\%s", m_curDevice.deviceName,ffd.cFileName,ffd1.cFileName);
						nFindType = 3;
						CloseHandle(hFind1) ;
					  }
				  }
				  else
				  {
					  memset(szDir1, 0, MAX_PATH);
					  sprintf(szDir1, "%s\\Content\\0000000000000000\\%s\\00050000\\*", m_curDevice.deviceName,ffd.cFileName);
					  if( stat( szDir1, &st ) == 0 && S_ISDIR( st.st_mode ) )
					  {
						   hFind1 = FindFirstFile(szDir1, &ffd1);
						  if (hFind1 != INVALID_HANDLE_VALUE && !(ffd1.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
						  {
							memset(szDir1, 0, MAX_PATH);
							sprintf(szDir1, "%s\\Content\\0000000000000000\\%s\\00050000\\%s", m_curDevice.deviceName,ffd.cFileName,ffd1.cFileName);
							nFindType = 4;
							CloseHandle(hFind1) ;
						  }
					  }
				  }
			  }
		  }
		  if(nFindType != 0)
		  {
				BYTE  m_pReadBuf[ 0x1792 + 2 ];
				m_pReadBuf[ 0 ] = '\0';
				m_pReadBuf[ 1 ] = '\0'; 
				hFind1 = CreateFile(szDir1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );  

				if( hFind1 != INVALID_HANDLE_VALUE )
				{        
					DWORD NChars;
					if( ReadFile( hFind1, m_pReadBuf, 0x1792, &NChars, NULL ))
					{

						GameNode *pGNode;
						pGNode = (GameNode *)malloc( sizeof(GameNode));
						memset(pGNode, 0,  sizeof(GameNode));

						ofstream out2;
						m_pReadBuf[ NChars ] = '\0';
						m_pReadBuf[ NChars + 1] = '\0';


						char *ptr;
						size_t length;

						//================================== 提取ID begin ==================================================
						ptr =  (CHAR *)m_pReadBuf + nTitleID;
						swprintf(pGNode->strTitleID, L"%-8X", ReadUInt32(ptr));
						//================================== 提取ID end ==================================================


						//================================== 提取标题 begin ==================================================
						ptr =  (CHAR *)m_pReadBuf + nContentTitle;
						int n = 0;
						int j =0;
						while(n < 0x40 )
						{
							BYTE bChar = (BYTE)ptr[n++];
							if(bChar != 0)
							{
								pGNode->strGameTitle[j++] = (CHAR)bChar;
							}
						}
						pGNode->strGameTitle[j] = '\0';
						wcscpy(pGNode->strName,pGNode->strGameTitle);

						//================================== 提取标题 end ==================================================

						memset(pGNode->strPath, 0, MAX_PATH);

						switch(nFindType)
						{
						case 1:
							sprintf(pGNode->strPath, "Content\\0000000000000000\\%s\\000D0000\\%s", ffd.cFileName,ffd1.cFileName);
							break;
						case 2:
							sprintf(pGNode->strPath, "Content\\0000000000000000\\%s\\000D0000\\%s", ffd.cFileName,ffd1.cFileName);
							break;
						case 3:
							sprintf(pGNode->strPath, "Content\\0000000000000000\\%s\\00070000\\%s", ffd.cFileName,ffd1.cFileName);
							break;
						case 4:
							sprintf(pGNode->strPath, "Content\\0000000000000000\\%s\\00050000\\%s", ffd.cFileName,ffd1.cFileName);
							break;
						}
						m_GameList.push_back(*pGNode);
					}
					CloseHandle(hFind1) ;
				}
		  }
      }
   }
   while (FindNextFile(hFind, &ffd) != 0);
   FindClose( hFind );
}

//--------------------------------------------------------------------------------------
// Name: LoadGameList
// Desc: 当前目录下的Hidden目录下的第一层目录加载都向量列表里
//--------------------------------------------------------------------------------------
VOID LoadGameList()
{
	char strFind[MAX_PATH];
	memset(strFind, 0, MAX_PATH);
	sprintf(strFind, "%s\\hidden\\*", m_curDevice.deviceName);

	char lpNewNameBuf[MAX_PATH];
	char lpGameName[MAX_PATH];
	WIN32_FIND_DATA wfd;
	HANDLE hFind;

	// 清空游戏列表
	m_GameList.clear();
	hFind = FindFirstFile( strFind, &wfd );

	HANDLE hFileFind;
    struct stat st;
	if( INVALID_HANDLE_VALUE != hFind )
	{
		do
		{
            // debug:修正存档的属性的问题 date:2010-02-24
			if(FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
            {
				memset(lpNewNameBuf, 0, MAX_PATH);
				sprintf(lpNewNameBuf, "%s\\hidden\\%s\\default.xex", m_curDevice.deviceName,wfd.cFileName);
				HANDLE hFile = CreateFile(lpNewNameBuf,    // file to open
					GENERIC_READ,           // open for reading
					FILE_SHARE_READ,        // share for reading
					NULL,                   // default security
					OPEN_EXISTING,          // existing file only
					FILE_FLAG_OVERLAPPED,   // overlapped operation
					NULL);                  // no attr. template

				if (hFile != INVALID_HANDLE_VALUE)
				{
					GameNode *pGlist;
					pGlist=(GameNode *)malloc( sizeof(GameNode));
					memset(pGlist, 0,  sizeof(GameNode));

					memset(lpNewNameBuf, 0, MAX_PATH);
					sprintf(lpNewNameBuf, "%s\\hidden\\%s\\default.txt", m_curDevice.deviceName,wfd.cFileName);
					if(getGameTitle(lpNewNameBuf,lpGameName))
					{
						memset(lpNewNameBuf, 0, MAX_PATH);
						sprintf(lpNewNameBuf, "%s",lpGameName);
						mbstowcs(pGlist->strName,lpNewNameBuf,strlen(lpNewNameBuf));
					}
					else
					{
						ConvertFileName(pGlist->strName,wfd.cFileName,m_curDevice.isUtf8);
					}

					pGlist->ftCreationTime = wfd.ftCreationTime;
					CHAR   strTitleImg[MAX_PATH];
					sprintf(strTitleImg, "file://%s/hidden/%s/default.png", m_curDevice.deviceName,wfd.cFileName);
					mbstowcs(pGlist->strTitleImagePath,strTitleImg,strlen(strTitleImg));

					sprintf(pGlist->strFileName, "%s", wfd.cFileName);

                    memset(pGlist->strPath, 0, MAX_PATH);
					sprintf(pGlist->strPath, "%s\\hidden\\%s\\default.xex", m_curDevice.deviceName,wfd.cFileName);

                    // 获得xex文件的头文件信息
                    XEXParse(pGlist);

					//==================================== 查找是否存在游戏背景图 begin ================================
					bool findAll = true;
					CHAR   strImg[MAX_PATH];
					memset(strImg, 0, MAX_PATH);
					sprintf(strImg, "%s\\hidden\\%s\\default_wall.png", m_curDevice.deviceName,wfd.cFileName);

					//hFileFind = CreateFile( strImg, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );    
					//if( hFileFind == INVALID_HANDLE_VALUE )
					//{        
					//	findAll = false;
					//}
					//else
					//{
					//	memset(strImg, 0, MAX_PATH);
					//	sprintf(strImg, "file://%s/hidden/%s/default_wall.png", m_curDevice.deviceName,wfd.cFileName);
					//	mbstowcs(pGlist->strWallPath,strImg,strlen(strImg));
					//}
					//CloseHandle(hFileFind);

                    if(FileExistsA(strImg))
                    {
                        memset(strImg, 0, MAX_PATH);
						sprintf(strImg, "file://%s/hidden/%s/default_wall.png", m_curDevice.deviceName,wfd.cFileName);
						mbstowcs(pGlist->strWallPath,strImg,strlen(strImg));
                    }
                    else
                    {
                        findAll = false;
                    }

					CHAR   strIco[MAX_PATH];
					memset(strIco, 0, MAX_PATH);
					sprintf(strIco, "%s\\hidden\\%s\\default_ico.png", m_curDevice.deviceName,wfd.cFileName);

					//hFileFind = CreateFile( strIco, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );    
					//if( hFileFind = INVALID_HANDLE_VALUE )
					//{
					//	findAll = false;
					//}
					//else
					//{
					//	memset(strIco, 0, MAX_PATH);
					//	sprintf(strIco, "file://%s/hidden/%s/default_ico.png", m_curDevice.deviceName,wfd.cFileName);
					//	mbstowcs(pGlist->strIcoPath,strIco,strlen(strIco));
					//}
					//CloseHandle(hFileFind);

                    // 判断是否存在，如果不存在封面图话，将加入下载栈中 date:2010-03-12 by:EME
                    if(FileExistsA(strIco))
                    {
						memset(strIco, 0, MAX_PATH);
						sprintf(strIco, "file://%s/hidden/%s/default_ico.png", m_curDevice.deviceName,wfd.cFileName);
						mbstowcs(pGlist->strIcoPath,strIco,strlen(strIco));
                    }
                    else
                    {
         //               DownNode *pDownNode;
					    //pDownNode=(DownNode *)malloc( sizeof(DownNode));
					    //memset(pDownNode, 0,  sizeof(DownNode));

         //               memset(pDownNode->strRemotePath, 0, MAX_PATH);
         //               sprintf(pDownNode->strRemotePath, "%s.png", pGlist->strTitleID);

         //               memset(pDownNode->strLocalPath, 0, MAX_PATH);
         //               sprintf(pDownNode->strLocalPath, "%s\\hidden\\%s\\%s", m_curDevice.deviceName,wfd.cFileName,pDownNode->strRemotePath);

         //               if(!ExtInList(pDownNode,&m_DownList))
         //               {
         //                   m_DownList.push_back(*pDownNode);
         //               }
         //               else
         //               {
         //                   memset(pDownNode, 0,  sizeof(DownNode));
         //               }
                    }

					pGlist->bIsRegion = findAll;
					//==================================== 查找是否存在游戏背景图 end ================================

					m_GameList.push_back(*pGlist);
				}		
				CloseHandle(hFile);
			}

		} while( FindNextFile( hFind, &wfd ) );

		FindClose( hFind );
	}
}

//--------------------------------------------------------------------------------------
// Name: class CGameList
// Desc: List控件的模板类
//--------------------------------------------------------------------------------------
class CGameList : public CXuiListImpl
{
	// Message map. Here we tie messages to message handlers.
	XUI_BEGIN_MSG_MAP()
		XUI_ON_XM_GET_SOURCE_TEXT( OnGetSourceText )
		XUI_ON_XM_GET_ITEMCOUNT_ALL( OnGetItemCountAll )
	XUI_END_MSG_MAP()

	//----------------------------------------------------------------------------------
	// Returns the number of items in the list.
	//----------------------------------------------------------------------------------
	HRESULT OnGetItemCountAll( XUIMessageGetItemCount* pGetItemCountData, BOOL& bHandled )
	{
		// edit:只绑定一页的数据 date:2009-11-18 by:EME
		int nCount = m_GameList.size() - m_nPageSize * (m_nCurPage - 1);
		pGetItemCountData->cItems = nCount > m_nPageSize ? m_nPageSize : nCount;
		bHandled = TRUE;
		return S_OK;
	}

	//----------------------------------------------------------------------------------
	// Returns the text for the items in the list.
	//----------------------------------------------------------------------------------
	HRESULT OnGetSourceText( XUIMessageGetSourceText* pGetSourceTextData, BOOL& bHandled )
	{
		if( pGetSourceTextData->bItemData && pGetSourceTextData->iItem >= 0 )
		{
			pGetSourceTextData->szText = m_GameList[m_nPageSize * (m_nCurPage - 1) + pGetSourceTextData->iItem].strName;
			bHandled = TRUE;
		}
		return S_OK;
	}

public:

	// Define the class. The class name must match the ClassOverride property
	// set for the scene in the UI Authoring tool.
	XUI_IMPLEMENT_CLASS( CGameList, L"GameList", XUI_CLASS_LIST )
};


//--------------------------------------------------------------------------------------
// Name: class CMyMainScene
// Desc: 场景模板类
//--------------------------------------------------------------------------------------
class CMyMainScene : public CXuiSceneImpl
{
	// Control and Element wrapper objects.
	CXuiTextElement m_Value;
	CXuiTextElement m_Page;
	CXuiList m_listDevice;
	CXuiList m_listGameType;
	CXuiList m_listMenu;
	CXuiList m_listLanguage;

	CXuiImageElement m_GameImage;
	CXuiImageElement m_BottomImage;
	CXuiImageElement m_AppWallImage;
	CXuiImageElement m_GameIcoImage;

	CXuiTextElement m_labelValueTitleId;
	CXuiTextElement m_labelValueTitleName;

	CXuiControl m_panelMenu;
	CXuiTextElement m_labelValueInfo;

	WCHAR  wszPageText[100];


	CXuiImageElement m_ImageA;
	CXuiImageElement m_ImageB;
	CXuiImageElement m_ImageY;
	CXuiImageElement m_ImageLB;
	CXuiImageElement m_ImageRB;

	CXuiTextElement m_labelA;
	CXuiTextElement m_labelB;
	CXuiTextElement m_labelY;

	CXuiTextElement m_labelIP;
	CXuiTextElement m_labelArcDescription;

    CXuiTextElement m_labelSmc;
    CXuiTextElement m_labelSpace;

	#define NONE 0
	#define INFO 1
	UINT m_nManagerOption;


	// Message map. Here we tie messages to message handlers.
	XUI_BEGIN_MSG_MAP()
		XUI_ON_XM_INIT( OnInit )
		XUI_ON_XM_KEYDOWN( OnKeyDown ) 
		XUI_ON_XM_NOTIFY_SELCHANGED( OnNotifySelChanged )
		XUI_ON_XM_NOTIFY_PRESS( OnNotifyPress )
        XUI_ON_XM_TIMER( OnTimer )
	XUI_END_MSG_MAP()

    HRESULT OnTimer( XUIMessageTimer *pTimer, BOOL& bHandled )
    {
        WCHAR strInfo[1024];
        memset(strInfo, 0, 1024);
        float a = 0;
        temperaturen = &a;
        MeinSMC.GetTemps(temperaturen,true);
        //    temps[0] = CPU
        //          temps[1] = GPU
        //          temps[2] = EDRAM
        //          temps[3] = MB
        swprintf(strInfo, L"CPU: %.1f, GPU: %.1f",temperaturen[0],temperaturen[1],temperaturen[2],temperaturen[3]);
        m_labelSmc.SetText(strInfo);

        // 设置LED灯,温度过高时,显示：红红红绿
        if(temperaturen[1] > 30)
        {
            MeinSMC.SetLEDS(RED,RED,RED,GREEN);  
        }

	    return S_OK;
    }

	//--------------------------------------------------------------------------------------
	// Name: RefreshPageInfo
	// Desc: 刷新分页信息
	//--------------------------------------------------------------------------------------
	VOID RefreshPageInfo()
	{
		m_nCountPage = (m_GameList.size() * 1.0 / m_nPageSize - m_GameList.size() / m_nPageSize) > 0 ? ( m_GameList.size() / m_nPageSize + 1) : ( m_GameList.size() / m_nPageSize);
		m_nCurPage = 1;
		swprintf(wszPageText, L"%d/%d  [%d]", m_nCurPage,m_nCountPage,m_GameList.size());
		m_Page.SetText(wszPageText);
		m_List.DeleteItems(0,m_List.GetItemCount());
		m_nCurSel = 0;
		int nCount = m_GameList.size();
		if(nCount > 0)
		{
			m_Value.SetText( m_GameList[m_nCurSel].strName );
			SetGameWall();

			nCount = nCount > m_nPageSize ? m_nPageSize : nCount;
			m_List.InsertItems(0,nCount);
			m_List.SetCurSel(m_nCurSel);
		}
		else
		{
			m_Value.SetText(L"");
			m_GameImage.SetImagePath(L"");
		}
	}

	//--------------------------------------------------------------------------------------
	// Name: MountDevice
	// Desc: 挂载设备
	//--------------------------------------------------------------------------------------
	BOOL MountDevice(WCHAR* deviceNameW)
	{
		bool isOk = false;
		int nCount = sizeof(m_DeviceMappings)/sizeof(m_DeviceMappings[0]);
		for (int i = 0; i < nCount; i++)
		{
			if(_wcsicmp(m_DeviceMappings[i].deviceNameW,deviceNameW) == 0)
			{
				m_curDevice = m_DeviceMappings[i];
				isOk = true;
				break;
			}
		}

		if(!isOk)
		{
			m_curDevice = m_DeviceMappings[0];
		}

		LoadList();
		SortList(1);		// 排序
		RefreshPageInfo();
		
		memset(m_ConfigNode.strDevice,0,MAX_PATH); 
		wcsncat_s(m_ConfigNode.strDevice,m_curDevice.deviceNameW,wcslen(m_curDevice.deviceNameW));

        // 显示当前设备的容量信息
	    ULARGE_INTEGER FreeBytesAvailable;
	    ULARGE_INTEGER TotalNumberOfBytes;
	    ULARGE_INTEGER TotalNumberOfFreeBytes;
        WCHAR strInfo[1024];
        char szDir[MAX_PATH];
        memset(szDir, 0, MAX_PATH);
        sprintf(szDir, "%s\\", UnicodeToAnsi(deviceNameW));
	    GetDiskFreeSpaceEx(szDir,&FreeBytesAvailable,&TotalNumberOfBytes,&TotalNumberOfFreeBytes);
	
	    if (TotalNumberOfBytes.QuadPart > 1024*1024*1024)
	    {
            swprintf(strInfo, L"%s %0.1fGb of %0.1fGb free",deviceNameW,(float)(TotalNumberOfFreeBytes.QuadPart)/(1024.0f*1024.0f*1024.0f),(float)(TotalNumberOfBytes.QuadPart)/(1024.0f*1024.0f*1024.0f));
	    } 
        else 
        {
		    swprintf(strInfo,L"%s %0.1fMb of %0.1fMb free",deviceNameW,(float)(TotalNumberOfFreeBytes.QuadPart)/(1024.0f*1024.0f),(float)(TotalNumberOfBytes.QuadPart)/(1024.0f*1024.0f));
	    }
		m_labelSpace.SetText(strInfo);

		return isOk;
	}

	//--------------------------------------------------------------------------------------
	// Name: SetGameWall
	// Desc: 设置游戏背景图、读取ARC的信息
	//--------------------------------------------------------------------------------------
	VOID SetGameWall()
	{
		m_labelArcDescription.SetText(L"");

		if(m_ConfigNode.nGameType == 0)
		{
			if(!m_GameList[m_nCurSel].bIsRegion)
			{
				BYTE  m_pReadBuf[ BUFSIZE + 2 ];
				m_GameList[m_nCurSel].bIsRegion = true;
				CHAR cNxeartFile[MAX_PATH];
				sprintf(cNxeartFile, "%s\\hidden\\%s\\nxeart", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName);

				m_pReadBuf[ 0 ] = '\0';
				m_pReadBuf[ 1 ] = '\0'; 
				HANDLE m_hFile = CreateFile(cNxeartFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );  

				CHAR	strIcoPath[MAX_PATH];
				CHAR	strWallPath[MAX_PATH];

				if( m_hFile != INVALID_HANDLE_VALUE )
				{        
					DWORD NChars;
					if( !ReadFile( m_hFile, m_pReadBuf, BUFSIZE, &NChars, NULL ))
					{
						CloseHandle(m_hFile);
						return;
					}
					CloseHandle(m_hFile) ;

					ofstream out2;
					m_pReadBuf[ NChars ] = '\0';
					m_pReadBuf[ NChars + 1] = '\0';

					BYTE startCodeJpeg[] = {0xFF,0xD8};
					BYTE endCodeJpeg[] = {0xFF,0xD9};

					BYTE startCodePng[] = {0x89,0x50};
					BYTE endCodePng[] = {0xAE,0x42,0x60,0x82};

					char *ptr = (CHAR *)m_pReadBuf;
					char *ptrEnd = ptr + BUFSIZE + 2;
					char *start_ptr,*end_ptr;
					size_t length;

					//================================== 提取ID begin ==================================================
					ptr =  (CHAR *)m_pReadBuf + nTitleID;
					swprintf(m_GameList[m_nCurSel].strTitleID, L"%-8X", ReadUInt32(ptr));
					//================================== 提取ID end ==================================================


					//================================== 提取标题 begin ==================================================
					ptr =  (CHAR *)m_pReadBuf + nContentTitle;
					int n = 0;
					int j =0;
					while(n < 0x40 )
					{
						BYTE bChar = (BYTE)ptr[n++];
						if(bChar != 0)
						{
							m_GameList[m_nCurSel].strGameTitle[j++] = (CHAR)bChar;
						}
					}
					m_GameList[m_nCurSel].strGameTitle[j] = '\0';
					//================================== 提取标题 end ==================================================


					//================================== 提取小图 begin ==================================================
					if(m_GameList[m_nCurSel].strIcoPath[0] == 0)
					{
						ptr =  (CHAR *)m_pReadBuf + nContentImageSize;
						start_ptr = ptr;
						length = ReadUInt32(start_ptr);

						sprintf_s( strIcoPath,"%s\\hidden\\%s\\default_ico.png", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName );
						out2.open(strIcoPath, ios::out | ios::binary);
						out2.write(start_ptr + 4,length);
						out2.close();
					}
					//================================== 提取小图 end ==================================================


					//================================== 提取大图 begin ==================================================
					if(m_GameList[m_nCurSel].strWallPath[0] == 0)
					{
						bool isJpeg = true;
						length = 0;

						ptr =  (CHAR *)m_pReadBuf + nStart;
						start_ptr = ptr;

						// 判断格式(png、jpeg)
						if((BYTE)ptr[0] == startCodeJpeg[0] && (BYTE)ptr[1] == startCodeJpeg[1])
						{
							ptr += 2;
							while(ptr < ptrEnd)
							{
								if((BYTE)ptr[0] == endCodeJpeg[0] && (BYTE)ptr[1] == endCodeJpeg[1] && (BYTE)ptr[2] == 0 && (BYTE)ptr[3] == 0)
								{
									length = ptr - start_ptr + 2;
									break;
								}
								ptr += 2;
							}
						}
						else if((BYTE)ptr[0] == startCodePng[0] && (BYTE)ptr[1] == startCodePng[1])
						{
							ptr += 5;
							while(ptr < ptrEnd)
							{
								if((BYTE)ptr[0] == endCodePng[0] && (BYTE)ptr[1] == endCodePng[1] && (BYTE)ptr[2] == endCodePng[2] && (BYTE)ptr[3] == endCodePng[3])
								{
									length = ptr - start_ptr + 4;
									break;
								}
								ptr += 4;
							}
						}

						if(length != 0)
						{
							sprintf_s(strWallPath,"%s\\hidden\\%s\\default_wall.png", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName );
							out2.open(strWallPath, ios::out | ios::binary);

							int num;
							if(length > BlockLevel[0])
							{
								num = BlockLevel[0];
								out2.write(start_ptr,num);
								num += 2 * 0x1000;
								start_ptr += num;
								if(length > BlockLevel[1])
								{
									length -= num;
									num = BlockLevel[1] - BlockLevel[0] - 2 * 0x1000;
									out2.write(start_ptr,num);

									num += 0x1000;
									start_ptr += num;
									out2.write(start_ptr,length - num);
								}
								else
								{
									out2.write(start_ptr,length - num);
								}
							}
							else
							{
								out2.write(start_ptr,length);
							}
							out2.close();
						}
					}
					//================================== 提取大图 end ==================================================
				}

				sprintf(strIcoPath, "file://%s/hidden/%s/default_ico.png", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName);
				mbstowcs(m_GameList[m_nCurSel].strIcoPath,strIcoPath,strlen(strIcoPath));

				sprintf(strWallPath, "file://%s/hidden/%s/default_wall.png", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName);
				mbstowcs(m_GameList[m_nCurSel].strWallPath,strWallPath,strlen(strWallPath));
			}

			// 设置背景图
			if(m_ConfigNode.nShowWall && !m_ConfigNode.nShowNewWall)
			{
				WCHAR aa[MAX_PATH];
				memset(aa,0,MAX_PATH); 
				wcsncat_s(aa,m_GameList[m_nCurSel].strWallPath,wcslen(m_GameList[m_nCurSel].strWallPath));
		
				m_WallImage.SetImagePath(m_GameList[m_nCurSel].strWallPath);
			}

			// 设置封面图
			m_GameImage.SetImagePath(m_GameList[m_nCurSel].strTitleImagePath);

			// 设置小图
			m_GameIcoImage.SetImagePath(m_GameList[m_nCurSel].strIcoPath);
		}
		else
		{
			m_GameIcoImage.SetImagePath(L"");

			char mntpth[] = "dice";
			UINT32 ret;

			unmountCon(mntpth);
			ret = mountCon(mntpth, m_curDevice.deviceName, m_GameList[m_nCurSel].strPath);

			if(ret == 0)
			{
				// 解析xml，读取信息
				ATG::XmlFileParser xmlFile;
				ArcadeInfo ArcadeInfoNode;						// 当前选中的arc信息
				//parser.RegisterSAXCallbackInterface( &xmlFile );
				HRESULT hr = xmlFile.LoadXMLFile( "dice:\\ArcadeInfo.xml",&ArcadeInfoNode,1);

				if( SUCCEEDED( hr ) )
				{
					// 设置小图
					m_GameIcoImage.SetImagePath(ArcadeInfoNode.strImagePath);

					//m_labelArcName.SetText(ArcadeInfoNode.strName );
					m_Value.SetText(ArcadeInfoNode.strName );
					m_labelArcDescription.SetText(ArcadeInfoNode.strDescription);
				}
				m_GameImage.SetImagePath(L"file://game:/media/XuiLocale.xzp#Media\\Xui\\arcade.jpg");
			}
		}

		m_labelValueTitleId.SetText(m_GameList[m_nCurSel].strTitleID);
		m_labelValueTitleName.SetText(m_GameList[m_nCurSel].strGameTitle);
	}

	//----------------------------------------------------------------------------------
	// 刷新设备列表（暂不支持获得各个设备的连接状态）
	//----------------------------------------------------------------------------------
	VOID RefashDeviceList()
	{
		int nIndex = 0;
		m_listDevice.DeleteItems(0,m_listDevice.GetItemCount());
		RefashDevice();
		int nCount = sizeof(m_DeviceMappings)/sizeof(m_DeviceMappings[0]);
		for (int i = 0; i < nCount; i++)
		{
			if(m_DeviceMappings[i].isSuccess)
			{
				m_listDevice.InsertItems(nIndex,1);
				m_listDevice.SetText(nIndex++,m_DeviceMappings[i].deviceNameW);
			}
		}
	}


	//----------------------------------------------------------------------------------
	// Performs initialization tasks - retrieves controls.
	//----------------------------------------------------------------------------------
	HRESULT OnInit( XUIMessageInit* pInitData, BOOL& bHandled )
	{
		// Retrieve controls for later use.
		GetChildById( L"labelValue", &m_Value );
		GetChildById( L"listDevice", &m_listDevice );
		GetChildById( L"listGameType", &m_listGameType );
		GetChildById( L"listMenu", &m_listMenu );
		GetChildById( L"listGames", &m_List );
		GetChildById( L"listLanguage", &m_listLanguage );
		GetChildById( L"AppWallImage", &m_AppWallImage );
		GetChildById( L"WallImage", &m_WallImage );
		GetChildById( L"GameImage", &m_GameImage );
		GetChildById( L"GameIcoImage", &m_GameIcoImage );
		GetChildById( L"BottomImage", &m_BottomImage );
		GetChildById( L"labelPage", &m_Page );
		GetChildById( L"labelValueTitleId", &m_labelValueTitleId );
		GetChildById( L"labelValueTitleName", &m_labelValueTitleName );
		GetChildById( L"panelMenu", &m_panelMenu );
		GetChildById( L"labelValueInfo", &m_labelValueInfo );
		GetChildById( L"ImageA", &m_ImageA );
		GetChildById( L"ImageB", &m_ImageB );
		GetChildById( L"ImageLB", &m_ImageLB );
		GetChildById( L"ImageRB", &m_ImageRB );
		GetChildById( L"ImageY", &m_ImageY );
		GetChildById( L"labelA", &m_labelA );
		GetChildById( L"labelB", &m_labelB );
		GetChildById( L"labelY", &m_labelY );

		GetChildById( L"labelIP", &m_labelIP );
		GetChildById( L"labelArcDescription", &m_labelArcDescription );
        GetChildById( L"labelSmc", &m_labelSmc );
        GetChildById( L"labelSpace", &m_labelSpace );
        
		m_ImageA.SetImagePath(L"file://game:/media/XuiLocale.xzp#Media\\Xui\\A.png");
		m_ImageB.SetImagePath(L"file://game:/media/XuiLocale.xzp#Media\\Xui\\B.png");
		m_ImageLB.SetImagePath(L"file://game:/media/XuiLocale.xzp#Media\\Xui\\LB.png");
		m_ImageRB.SetImagePath(L"file://game:/media/XuiLocale.xzp#Media\\Xui\\RB.png");
		m_ImageY.SetImagePath(L"file://game:/media/XuiLocale.xzp#Media\\Xui\\Y.png");
		m_BottomImage.SetImagePath(L"file://game:/media/XuiLocale.xzp#Media\\Xui\\BottomImage.png");


		if(m_ConfigNode.nShowNewWall)
		{
			m_AppWallImage.SetImagePath(m_ConfigNode.strWallPath);
		}
		else
		{
			m_AppWallImage.SetImagePath(m_strAppWallPath);
		}

		LPCWSTR str = m_listMenu.GetText(3);
		wcsncpy_s( m_strShowWall, str, wcslen(str) );

		str = m_listMenu.GetText(4);
		wcsncpy_s( m_strShowNewWall, str, wcslen(str) );
		
		if(m_ConfigNode.nShowWall)
		{
			m_listMenu.SetText(3,StrAdd(m_strShowWall,L"[OFF]"));
		}
		else
		{
			m_listMenu.SetText(3,StrAdd(m_strShowWall,L"[ON]"));
		}

		if(m_ConfigNode.nShowNewWall)
		{
			m_listMenu.SetText(4,StrAdd(m_strShowNewWall,L"[OFF]"));
		}
		else
		{
			m_listMenu.SetText(4,StrAdd(m_strShowNewWall,L"[ON]"));
		}

		// 显示IP
		unsigned long ulIp = ntohl( m_xnaddr.ina.S_un.S_addr );
		WCHAR strIp[256];
		swprintf(strIp, L"%lu.%lu.%lu.%lu", (ulIp >> 24) & 255, (ulIp >> 16) & 255, (ulIp >> 8) & 255, ulIp & 255);
		m_labelIP.SetText(strIp);


		// add:默认读取配置中的设备 date:2009-12-23 by:EME
		MountDevice(m_ConfigNode.strDevice);

		m_nManagerOption = NONE;
		m_labelValueInfo.SetOpacity(0);
		m_panelMenu.SetOpacity(0);
		m_listMenu.SetOpacity(0);
		m_listDevice.SetOpacity(0);
		m_listGameType.SetOpacity(0);
		m_listLanguage.SetOpacity(0);
		m_List.SetOpacity(1);
		m_List.SetFocus(XUSER_INDEX_ANY);

		m_ImageB.SetOpacity(0);
		m_labelB.SetOpacity(0);

		//m_labelArcName.SetOpacity(0);
		//m_labelArcDescription.SetOpacity(0);

        // 定时刷新SMC温度信息(1秒)
        SetTimer(0,1000);

        MeinSMC.SetFanSpeed(1,FSpeed); 

		return S_OK;
	}

	//----------------------------------------------------------------------------------
	// Updates the UI when the list selection changes.
	//----------------------------------------------------------------------------------
	HRESULT OnNotifySelChanged( HXUIOBJ hObjSource, XUINotifySelChanged* pNotifySelChangedData, BOOL& bHandled )
	{
		if( hObjSource == m_List )
		{
			m_nCurSel = m_nPageSize * (m_nCurPage - 1) + m_List.GetCurSel();
			m_Value.SetText( m_GameList[m_nCurSel].strName );

			SetGameWall();
			bHandled = TRUE;
		}

		return S_OK;
	}

	HRESULT OnNotifyPress( HXUIOBJ hObjPressed, BOOL& bHandled )
	{
		// edit:使用SONIC3D封装的api date:2009-12-23 by:EME
		if( hObjPressed == m_List )
		{
			bHandled = TRUE;

			if(m_ConfigNode.nGameType == 0)
			{
				launchX( m_GameList[m_nCurSel].strPath);
			}
			else
			{
				launchX("dice:\\default.xex");
			}
		}
		else if( hObjPressed == m_listDevice )
		{
			MountDevice((LPWSTR)(m_listDevice.GetText(m_listDevice.GetCurSel())));
		}
		else if( hObjPressed == m_listGameType )
		{
			m_ConfigNode.nGameType = m_listGameType.GetCurSel();
			MountDevice(m_ConfigNode.strDevice);
		}
		else if( hObjPressed == m_listMenu )
		{
			switch(m_listMenu.GetCurSel())
			{
				case 0:				// 显示设备列表
					m_listMenu.SetOpacity(0);
					RefashDeviceList();
					m_listDevice.SetOpacity(1);
					m_listDevice.SetFocus(XUSER_INDEX_ANY);
					break;
				case 1:				// 显示游戏类型选择
					m_listMenu.SetOpacity(0);
					m_listGameType.SetOpacity(1);
					m_listGameType.SetFocus(XUSER_INDEX_ANY);
					break;
				case 2:
					m_listMenu.SetOpacity(0);
					m_listLanguage.SetOpacity(1);
					m_listLanguage.SetFocus(XUSER_INDEX_ANY);
					break;
				case 3:
					if(m_ConfigNode.nShowWall)
					{
						m_ConfigNode.nShowWall = 0;
						m_WallImage.SetImagePath(L"");
						m_listMenu.SetText(3,StrAdd(m_strShowWall,L"[ON]"));
					}
					else
					{
						m_ConfigNode.nShowWall = 1;
						if(m_List.GetCurSel() >= 0)
						{
							m_WallImage.SetImagePath(m_GameList[m_nCurSel].strWallPath);
						}
						m_listMenu.SetText(3,StrAdd(m_strShowWall,L"[OFF]"));
					}
					break;
				case 4:
					if(m_ConfigNode.nShowNewWall)
					{
						m_ConfigNode.nShowNewWall = 0;
						m_WallImage.SetImagePath(L"");
						m_listMenu.SetText(4,StrAdd(m_strShowNewWall,L"[ON]"));

						memset(m_ConfigNode.strWallPath,0,MAX_PATH); 
						m_AppWallImage.SetImagePath(m_strAppWallPath);
					}
					else
					{
						m_ConfigNode.nShowNewWall = 1;
						if(m_List.GetCurSel() >= 0)
						{
							memset(m_ConfigNode.strWallPath,0,MAX_PATH); 
							wcsncpy_s( m_ConfigNode.strWallPath,m_GameList[m_nCurSel].strWallPath,wcslen(m_GameList[m_nCurSel].strWallPath));
							m_AppWallImage.SetImagePath(m_ConfigNode.strWallPath);
						}
						m_listMenu.SetText(4,StrAdd(m_strShowNewWall,L"[OFF]"));
					}
					m_WallImage.SetImagePath(m_GameList[m_nCurSel].strWallPath);
					break;
               case 5:				// 设置风扇速度
					break;
				case 6:				// 返回XDK界面
					SaveConfig();
					launchX( XLAUNCH_KEYWORD_DEFAULT_APP );
					break;
				case 7:				// 返回DASH界面
					SaveConfig();
					launchX( XLAUNCH_KEYWORD_DASH);
					break;
			}
		}
		else if( hObjPressed == m_listLanguage )
		{
			DWORD dwLanguage;
			switch(m_listLanguage.GetCurSel())
			{
				case 0:	
					dwLanguage = XGetLanguage();
					if(dwLanguage != 2 || dwLanguage != 8 || dwLanguage != 10)
					{
						dwLanguage = 0;
					}
					break;
				case 1:
					dwLanguage = 0;
					break;
				case 2:
					dwLanguage = 2;
					break;
				case 3:
					dwLanguage = 8;
					break;
				case 4:
					dwLanguage = 10;
					break;
			}

			if( dwLanguage>=sizeof( LocaleLanguage )/sizeof( LocaleLanguage[0] ) )
			{
				// 超出范围话使用默认语言(英文)
				dwLanguage = 0;
			}
			m_ConfigNode.nLanguage = dwLanguage;
			XuiSetLocale( LocaleLanguage[dwLanguage] );
			XuiApplyLocale( m_hObj, NULL );

			LPCWSTR str = m_listMenu.GetText(3);
			wcsncpy_s( m_strShowWall, str, wcslen(str) );

			str = m_listMenu.GetText(4);
			wcsncpy_s( m_strShowNewWall, str, wcslen(str) );
			
			if(m_ConfigNode.nShowWall)
			{
				m_listMenu.SetText(3,StrAdd(m_strShowWall,L"[OFF]"));
			}
			else
			{
				m_listMenu.SetText(3,StrAdd(m_strShowWall,L"[ON]"));
			}

			if(m_ConfigNode.nShowNewWall)
			{
				m_listMenu.SetText(4,StrAdd(m_strShowNewWall,L"[OFF]"));
			}
			else
			{
				m_listMenu.SetText(4,StrAdd(m_strShowNewWall,L"[ON]"));
			}

			switch(dwLanguage)
			{
			case 2:
				m_ConfigNode.nOemCode = IDS_JP;
				break;
			case 7:
				m_ConfigNode.nOemCode = IDS_KOREAN;
				break;
			case 8:
				m_ConfigNode.nOemCode = IDS_CHT;
				break;
			default:
				m_ConfigNode.nOemCode = IDS_CHS;
				break;
			}
			CP_Init(m_ConfigNode.nOemCode);
			MountDevice(m_ConfigNode.strDevice);
		}
		return S_OK;
	}

	HRESULT OnKeyDown(XUIMessageInput *pInputData,BOOL &bHandled)
	{
		CXuiControl menuItemCtrl;
		CXuiControl deviceItemCtrl;
		CXuiControl m_listGameTypeCtrl;
		CXuiControl gameItemCtrl;
		CXuiControl languageItemCtrl;


		m_listMenu.GetCurSel(&menuItemCtrl);
		m_listDevice.GetCurSel(&deviceItemCtrl);
		m_listGameType.GetCurSel(&m_listGameTypeCtrl);
		m_List.GetCurSel(&gameItemCtrl);
		m_listLanguage.GetCurSel(&languageItemCtrl);

		bool isPage = false;
		if(m_nManagerOption == NONE)
		{
			if ( deviceItemCtrl == deviceItemCtrl.GetFocus(XUSER_INDEX_ANY))
			{
				// 返回主菜单
				if ( pInputData->dwKeyCode == VK_PAD_B)
				{
					m_ImageB.SetOpacity(1);
					m_labelB.SetOpacity(1);

					m_ImageY.SetOpacity(0);
					m_labelY.SetOpacity(0);

					m_listDevice.SetOpacity(0);
					m_listMenu.SetOpacity(1);
					m_listMenu.SetFocus(XUSER_INDEX_ANY);
				}
			}
			else if( m_listGameTypeCtrl == deviceItemCtrl.GetFocus(XUSER_INDEX_ANY))
			{
				// 返回主菜单
				if ( pInputData->dwKeyCode == VK_PAD_B)
				{

					m_ImageB.SetOpacity(1);
					m_labelB.SetOpacity(1);

					m_ImageY.SetOpacity(0);
					m_labelY.SetOpacity(0);

					m_listGameType.SetOpacity(0);
					m_listMenu.SetOpacity(1);
					m_listMenu.SetFocus(XUSER_INDEX_ANY);
				}
			}
			else if ( menuItemCtrl == menuItemCtrl.GetFocus(XUSER_INDEX_ANY))
			{
				// 返回界面
				if ( pInputData->dwKeyCode == VK_PAD_B)
				{
					m_ImageB.SetOpacity(0);
					m_labelB.SetOpacity(0);

					m_ImageY.SetOpacity(1);
					m_labelY.SetOpacity(1);

					m_panelMenu.SetOpacity(0);
					m_listMenu.SetOpacity(0);
					m_List.SetFocus(XUSER_INDEX_ANY);
				}

                // 设置风扇速度
                if ( m_listMenu.GetCurSel() == 5)
                {
                    int OldSpeed = FSpeed; 
                    if ( pInputData->dwKeyCode == VK_PAD_DPAD_LEFT)
                    {
                        FSpeed += 5;
                        if(FSpeed > 100)
                        {
                            FSpeed = 100;
                        }
                    }
                    else if ( pInputData->dwKeyCode == VK_PAD_DPAD_RIGHT)
                    {
                        FSpeed -= 5;
                         if(FSpeed < 0)
                        {
                            FSpeed = 0;
                        }
                    }

                    if(OldSpeed != FSpeed)
                    {
                        MeinSMC.SetFanSpeed(1,FSpeed); 
                        WCHAR strInfo[256];
                        swprintf(strInfo, L"+风扇速度[%d%%]-",FSpeed);
                        m_listMenu.SetText(5,strInfo);
                    }

                    // if (FSpeed <= 40) 
                    //{ 
                    //        SpeedText = sprintfa(L"Auto"); 
                    //} 
                    //else 
                    //{ 
                    //        SpeedText = sprintfa(L"%d%%", FSpeed); 
                    //} 
  
                }
			}
			else if(languageItemCtrl == languageItemCtrl.GetFocus(XUSER_INDEX_ANY))
			{
				// 返回主菜单
				if ( pInputData->dwKeyCode == VK_PAD_B)
				{
					m_ImageB.SetOpacity(1);
					m_labelB.SetOpacity(1);

					m_ImageY.SetOpacity(0);
					m_labelY.SetOpacity(0);

					m_listLanguage.SetOpacity(0);
					m_listMenu.SetOpacity(1);
					m_listMenu.SetFocus(XUSER_INDEX_ANY);
				}
			}
			else
			{
				switch ( pInputData->dwKeyCode )
				{
				case VK_PAD_Y:										// 菜单
					m_ImageB.SetOpacity(1);
					m_labelB.SetOpacity(1);

					m_ImageY.SetOpacity(0);
					m_labelY.SetOpacity(0);

					m_listMenu.SetOpacity(1);
					m_listMenu.SetFocus(XUSER_INDEX_ANY);
					break;
				case VK_PAD_LSHOULDER:								// 上页
					if(m_nCurPage > 1)
					{
						m_nCurPage--;
						isPage = true;
					}
					break;
				case VK_PAD_RSHOULDER:								// 下页
					if(m_nCurPage < m_nCountPage)
					{
						m_nCurPage++;
						isPage = true;
					}
					break;
				case VK_PAD_B:										// 显示说明
					m_nManagerOption = INFO;
					m_ImageB.SetOpacity(1);
					m_labelB.SetOpacity(1);

					m_ImageA.SetOpacity(0);
					m_labelA.SetOpacity(0);
					m_ImageY.SetOpacity(0);
					m_labelY.SetOpacity(0);
					m_listMenu.SetOpacity(0);
					m_panelMenu.SetOpacity(1);
					m_labelValueInfo.SetOpacity(1);
					m_listMenu.SetFocus(XUSER_INDEX_ANY);
					break;
				case VK_PAD_X:										// 排序
					m_bSortLess = !m_bSortLess;
					MountDevice(m_ConfigNode.strDevice);
					break;
				case VK_PAD_BACK:									// 返回DASH界面
					SaveConfig();
					launchX( XLAUNCH_KEYWORD_DASH);
					break;
				}

				if(isPage)
				{
					swprintf(wszPageText, L"%d/%d  [%d]", m_nCurPage,m_nCountPage,m_GameList.size());
					m_Page.SetText(wszPageText);
					m_nCurSel = m_nPageSize * (m_nCurPage - 1) + m_List.GetCurSel();
					if(m_nCurSel > m_GameList.size() - 1)
					{
						m_nCurSel = m_GameList.size() - 1;
						m_List.SetCurSel(m_nCurSel -  m_nPageSize * (m_nCurPage - 1));
					}
					m_Value.SetText( m_GameList[m_nCurSel].strName );

					// edit设置游戏图片 date:2009-12-23 by:EME
					SetGameWall();
					m_List.DeleteItems(0,m_List.GetItemCount());
				}
			}
		}
		else if(m_nManagerOption == INFO)
		{
			// 返回界面
			if ( pInputData->dwKeyCode == VK_PAD_B)
			{
				m_ImageB.SetOpacity(0);
				m_labelB.SetOpacity(0);

				m_ImageA.SetOpacity(1);
				m_labelA.SetOpacity(1);
				m_ImageY.SetOpacity(1);
				m_labelY.SetOpacity(1);

				m_nManagerOption = NONE;
				m_labelValueInfo.SetOpacity(0);
				m_panelMenu.SetOpacity(0);
				m_List.SetFocus(XUSER_INDEX_ANY);
			}
		}
		bHandled = true;
		return S_OK;
	}


public:

	// Define the class. The class name must match the ClassOverride property
	// set for the scene in the UI Authoring tool.
	XUI_IMPLEMENT_CLASS( CMyMainScene, L"MyMainScene", XUI_CLASS_SCENE )
};

int HOverscan = 0;
int VOverscan = 0;

//--------------------------------------------------------------------------------------
// Main XUI host class. It is responsible for registering scene classes and provide
// basic initialization, scene loading and rendering capability.
//--------------------------------------------------------------------------------------
class CMyApp : public CXuiModule
{
protected:
	// Override RegisterXuiClasses so that CMyApp can register classes.
	virtual HRESULT RegisterXuiClasses();

	// Override UnregisterXuiClasses so that CMyApp can unregister classes. 
	virtual HRESULT UnregisterXuiClasses();

public:
    DWORD                               dwWidth;
    DWORD                               dwHeight;

	void GetVidMode();
	HRESULT Render();
};


//--------------------------------------------------------------------------------------
// Name: RegisterXuiClasses()
// Desc: 注销全部的场景类
//--------------------------------------------------------------------------------------
HRESULT CMyApp::RegisterXuiClasses()
{
	// Register any other classes necessary for the app/scene
	HRESULT hr = CMyMainScene::Register();
	if( FAILED( hr ) )
		return hr;

	hr = CGameList::Register();
	if( FAILED( hr ) )
		return hr;

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: UnregisterXuiClasses()
// Desc: 注销使用的场景
//--------------------------------------------------------------------------------------
HRESULT CMyApp::UnregisterXuiClasses()
{
	CGameList::Unregister();
	CMyMainScene::Unregister();
	return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: RegisterXuiClasses
// Desc: Registers all the scene classes.
//--------------------------------------------------------------------------------------
void CMyApp::GetVidMode()
{
    XVIDEO_MODE VideoMode;
    XGetVideoMode( &VideoMode );

	dwWidth = VideoMode.dwDisplayWidth;
	dwHeight = VideoMode.dwDisplayHeight;
}

HRESULT CMyApp::Render()
{

	ASSERT(m_hDC != NULL);
    HRESULT hr = XuiRenderBegin(m_hDC, D3DCOLOR_ARGB(255, 0, 0, 0));
    if (FAILED(hr))
        return hr;


    D3DXMATRIX matOrigView;
    XuiRenderGetViewTransform( m_hDC, &matOrigView );

    // scale depending on the height of the render target
    D3DXMATRIX matView;
	int NewWidth = dwWidth - (HOverscan * 2);
	int NewHeight = dwHeight - (VOverscan * 2);
    D3DXVECTOR2 vScaling = D3DXVECTOR2( NewWidth / 1280.0f, NewHeight / 720.0f );
	D3DXVECTOR2 vTranslation = D3DXVECTOR2( (float)HOverscan, (float)VOverscan );
    D3DXMatrixTransformation2D( &matView, NULL, 0.0f, &vScaling, NULL, 0.0f, &vTranslation );
    XuiRenderSetViewTransform( m_hDC, &matView );


	XUIMessage msg;
    XUIMessageRender msgRender;
    XuiMessageRender(&msg, &msgRender, m_hDC, 0xffffffff, XUI_BLEND_NORMAL);
    XuiSendMessage(m_hObjRoot, &msg);


    XuiRenderSetViewTransform( m_hDC, &matOrigView );

	XuiRenderEnd(m_hDC);

    XuiRenderPresent(m_hDC, NULL, NULL, NULL);
	Sleep(15);

    return S_OK;

}


void MountAll()
{
	m_DeviceMappings[0].isSuccess = (Mount( "Devkit","\\Device\\Harddisk0\\Partition1\\Devkit") == S_OK);
	m_DeviceMappings[1].isSuccess = (Mount( "Hdd","\\Device\\Harddisk0\\Partition1") == S_OK);
	m_DeviceMappings[2].isSuccess = (Mount( "Usb0","\\Device\\Mass0") == S_OK);
	m_DeviceMappings[3].isSuccess = (Mount( "Usb1","\\Device\\Mass1") == S_OK);
	m_DeviceMappings[4].isSuccess = (Mount( "Usb2","\\Device\\Mass2") == S_OK);
	m_DeviceMappings[5].isSuccess = (Mount( "Dvd","\\Device\\Cdrom0") == S_OK);
	m_DeviceMappings[6].isSuccess = (Mount( "Flash","\\Device\\Flash") == S_OK);
	m_DeviceMappings[7].isSuccess = (Mount( "HddX","\\Device\\Harddisk0\\SystemPartition") == S_OK);
}

bool InitNetwork()
{
	DWORD dwStatus = XNetGetEthernetLinkStatus();
	int m_bIsOnline = ( dwStatus & XNET_ETHERNET_LINK_ACTIVE ) != 0;

	if( !m_bIsOnline )
	{
		return false;
	}

	DWORD dwRet;
	do
	{
		dwRet = XNetGetTitleXnAddr( &m_xnaddr );
	} while( dwRet == XNET_GET_XNADDR_PENDING );

	XNetStartupParams xnsp;
	memset(&xnsp, 0, sizeof(xnsp));
	xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
	xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;

	INT iResult = XNetStartup( &xnsp );
	if( iResult != NO_ERROR )
	{
		return false;
	}

	WSADATA WsaData;
	iResult = WSAStartup( MAKEWORD( 2, 2 ), &WsaData );
	if( iResult != NO_ERROR )
	{
		return false;
	}

	return true;
}


//--------------------------------------------------------------------------------------
// Name: main()
// Desc: 主函数，程序的入口处
//--------------------------------------------------------------------------------------
VOID __cdecl main()
{
	_unlink("game:\\debug.log");
	_unlink(m_strConfigPath);

	InitNetwork();
	MountAll();

	CFtpServer FtpServer;
	// 数据socket端口[1025,9000]
	FtpServer.SetDataPortRange( 1025, 9000 ); 
	CFtpServer::UserNode *FtpUser = FtpServer.AddUser( "xbox", "xbox", "/");

	if( FtpUser ) 
	{
		FtpServer.SetUserPriv( FtpUser, CFtpServer::READFILE | CFtpServer::LIST| CFtpServer::WRITEFILE| CFtpServer::DELETEFILE| CFtpServer::CREATEDIR| CFtpServer::DELETEDIR );
		if( FtpServer.StartListening( INADDR_ANY, 21 ) )
		{
			FtpServer.StartAccepting();
		} 
	} 

	CMyApp app;
	app.GetVidMode(); 

	HRESULT hr = app.Init( XuiD3DXTextureLoader );
	if( FAILED( hr ) )
		ATG::FatalError( "Failed intializing application.\n" );

	m_ConfigNode.nShowWall = 1;
	m_ConfigNode.nOemCode = 936; // 默认简体936编码
	swprintf( m_ConfigNode.strDevice, L"Devkit");
	memset(m_ConfigNode.strWallPath,0,MAX_PATH); 
	m_ConfigNode.nShowNewWall = 0;
	m_ConfigNode.nGameType = 0;

	// add:读取配置文件 date:2009-12-30 by:chengang
	LoadConfig();

	// 注册字体文件
	hr = app.RegisterDefaultTypeface( L"Arial Unicode MS", L"file://game:/media/xarialuni.ttf" );
	if( FAILED( hr ) )
		ATG::FatalError( "Failed to register default typeface.\n" );

	// 载入所用的皮肤文件
	app.LoadSkin( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\simple_scene_skin.xur" );

	// 获取当前当前语言
	DWORD dwLanguage = XGetLanguage();

	if( dwLanguage>=sizeof( LocaleLanguage )/sizeof( LocaleLanguage[0] ) )
	{
		// 超出范围话使用默认语言(英文)
		dwLanguage = 0;
	}

	if(m_ConfigNode.nOemCode == 0)
	{
		switch(dwLanguage)
		{
		case 2:
			m_ConfigNode.nOemCode = IDS_JP;
			break;
		case 7:
			m_ConfigNode.nOemCode = IDS_KOREAN;
			break;
		case 8:
			m_ConfigNode.nOemCode = IDS_CHT;
			break;
		default:
			m_ConfigNode.nOemCode = IDS_CHS;
			break;
		}
	}

	if( m_ConfigNode.nLanguage < 0 || m_ConfigNode.nLanguage >= sizeof( LocaleLanguage )/sizeof( LocaleLanguage[0] ) )
	{
		dwLanguage = 0;
	}
	else if( m_ConfigNode.nLanguage > 0)
	{
		dwLanguage = m_ConfigNode.nLanguage;
	}

	// 默认英语显示
	switch(dwLanguage)
	{
	case 2:
	case 7:
	case 8:
	case 10:
		break;
	default:
		dwLanguage = 0;
		break;
	}

	m_ConfigNode.nLanguage = dwLanguage;
	// 方法1：设置当前XUI本地化
	XuiSetLocale( LocaleLanguage[dwLanguage] );

	// add:初始化编码 date:2009-12-29 by:EME
	CP_Init(m_ConfigNode.nOemCode);
	
	m_nPageSize = 13;
	app.LoadFirstScene( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\", L"XuiLocale_main.xur", NULL );    



    // 下载线程
    DWORD dwThreadId, dwThrdParam = 1; 
    HANDLE hDownThread = CreateThread( 
    NULL,                             // (this parameter is ignored)
    0,                                  // use default stack size  
    DownFileThread,             // thread function 
    NULL,                            // argument to thread function 
    0,                                  // use default creation flags 
    &dwThreadId);               // returns the thread identifier 

	app.Run();

	FtpServer.StopListening();
	app.Uninit();
}




/**********************************************************
函名名称: DownFileThread
描    述: 下载线程
输入参数: LPVOID pAram			传进来的指针参数(基本是要操作的对象指针)
输出参数: 
返 回 值: DWORD  (返回执行状态)
**********************************************************/
DWORD WINAPI DownFileThread(LPVOID pAram)
{

	CFtpClient* pFtpClient = &m_ftpClient;							// 上传操作对象指针
	SOCKET* pSockDate = &m_sockDateDownT;				// 数据传送socket
	SOCKET* pSockClient = &m_sockClientDownT;			// 命令socket
    string strValue,strTmp;
    FILE *ofp = NULL;

    *pSockClient = pFtpClient->ConnectFtp("192.168.73.1", 23, "test", "test");															// 连接FTP服务器
    int SIZE = pFtpClient->m_nSIZE;

	while(!*pSockClient)
    {
        Sleep(5000);
        *pSockClient = pFtpClient->ConnectFtp("192.168.73.1", 23, "test", "test");															// 连接FTP服务器
    }

    while(true)
    {
	    try
	    {
            while(m_DownList.size() == 0)
            {
                Sleep(5000);
            }
		    
		    int iCount;
		    long nFileSize = 0,nDownFileSize = 0;

		    if(!*pSockClient)
		    {
                *pSockClient = pFtpClient->ConnectFtp("192.168.73.1", 23, "test", "test");															// 连接FTP服务器
                if(!*pSockClient)
                {
			        goto end;
                }
		    }

            if(ofp != NULL)
            {
                fclose(ofp);
            }
            DownNode itemNode = m_DownList[0];
            m_DownList.pop_back();

            ofp = fopen(itemNode.strLocalPath,"wb");
            if(ofp==NULL)
            {
                goto end;
            }

		    long nCode = pFtpClient->SendCommand(*pSockClient, "REST", "0");
		    if (nCode == 202 || nCode != 350)				// 202 未执行命令，站点上的命令过多
		    {
			    Sleep(500);
			    goto end;
		    }

		    nCode = pFtpClient->SendCommand(*pSockClient, "REST", "100");
		    if (nCode == 202 || nCode != 350)				// 202 未执行命令，站点上的命令过多
		    {
			    Sleep(500);
			    goto end;
		    }

            nCode = pFtpClient->SendCommand(*pSockClient, "CWD", itemNode.strRemotePath);
		    if (nCode == 202 )						// 202 未执行命令，站点上的命令过多
		    {
			    Sleep(500);
			    goto end;
		    }
		    //else if(nCode != 250 )					// 设置服务器工作目录
		    //{
			   // strTmp = strObject;
      //          while (strObject.length()> 0) 
			   // {
				  //  if (strObject.find('/') <= 0)
				  //  {
					 //   strObject += "/";
				  //  }
      //              strTmp = strObject.substr(0,strObject.find('/')+1);
				  //  nCode = pFtpClient->SendCommand(*pSockClient, "CWD", strTmp);
				  //  if (nCode == 202 || nCode != 250 )						// 202 未执行命令，站点上的命令过多 // 路径不存在
				  //  {
					 //   Sleep(500);
					 //   goto end;
				  //  }
      //              strObject = strObject.substr(strObject.find('/') + 1, strObject.length());
			   // }
		    //}

		    // 记录要下载的文件的总大小
		    nCode = pFtpClient->SendCommand(*pSockClient, "SIZE", itemNode.strRemotePath);
		    if(nCode != 213)						// 发送请求，得到要下载文件的大小
		    {
			    goto end;
		    }
		    else
		    {
			    if (pFtpClient->m_strRecvLastInfo.length() < 4)
			    {
				    Sleep(200);
				    goto end;
			    }

			    // 获取文件大小
			    string temp;
                temp = pFtpClient->m_strRecvLastInfo.substr(4, pFtpClient->m_strRecvLastInfo.length() - 4);
			    string str = pFtpClient->m_strRecvLastInfo.substr(4, pFtpClient->m_strRecvLastInfo.length() - 6);
                nFileSize = atoi(temp.c_str());

			    if(nFileSize <= 0)
			    {
				    Sleep(200);
				    goto end;
			    }
		    }

            // 通知服务器传输的方式(返回:是否定位出错)
            // 发送连接请求，确认连接方式,得到数据通道的连接
		    if(!pFtpClient->SendCommand(*pSockClient, "TYPE", "I", 200) || !pFtpClient->SendCommand(*pSockClient, "PASV", "", 227))	
		    {
			    goto end;
		    }

		    // 数据传送socket
		    *pSockDate = pFtpClient->GetConnect(pFtpClient->GetHost(pFtpClient->m_strRecvLastInfo),pFtpClient->GetPort(pFtpClient->m_strRecvLastInfo));
		    if(!pFtpClient->SetFilePos(*pSockClient,0) || !pFtpClient->SendCommand(*pSockClient, "RETR", itemNode.strRemotePath, 150))													// 定位下载位置(重新下载) // 发送下载文件请求
		    {
                goto end;
		    }
            
            fseek(ofp,0x00,SEEK_SET);

		    long refilesize = nFileSize - nDownFileSize;												// 记录剩余的文件大小
		    TCHAR *szBuf = new TCHAR[SIZE];																// 定义缓存空间
		    memset(szBuf, 0, SIZE);
		    while(iCount = recv(*pSockDate, (char FAR *)szBuf, SIZE, 0))
		    {
                fwrite(szBuf ,1,iCount,ofp);
			    //pFile->Write(szBuf,iCount);		
			    refilesize -= iCount;																	// 记录剩余的文件大小
			    nFileSize += iCount;																	// 记录已经下载了多少
		    }
		    // 操作完成，善后工作
		    delete [] szBuf;

	    }
	    catch (...)
	    {
	    }

    end:
        {
		    // 关闭SOCKET
		    shutdown(*pSockDate, SD_RECEIVE);
		    closesocket(*pSockDate);
		    closesocket(*pSockClient);	
            fclose(ofp);
            ofp = NULL;
        }
    }
	
	return 0;
}

void XEXParse(GameNode * item)
{
    WCHAR strPath[256];


	swprintf(strPath, L"%s", item->strPath);

	FILE * fp  = fopen(item->strPath ,"rb");
	if (!fp)
		return;

	fseek(fp,0x14,SEEK_SET);
	int temp = 0;
	fread(&temp,4,1,fp);

	if (temp > 50)
	{
		fclose(fp);
		return;
	}

	xex_rec * recs = new xex_rec[temp];
	fread(recs,sizeof(xex_rec),temp,fp);

	int headeraddy = 0;
	for (int i = 0 ; i < temp ; i++)
	{
		if (recs[i].var1 == 0x00040006)
		{
			headeraddy = recs[i].var2;
		}
	}

	if (headeraddy > 0x30000 || headeraddy < 0x100)
	{
		fclose(fp);
		return;
	}
	fseek(fp,headeraddy,SEEK_SET);

	header_1 header;
	fread(&header,sizeof(header),1,fp);

	//DebugMsg("Got %08x, %08x, version %d, %d, Disk %d of %d",header.mediaid, header.titleid, header.version1, header.version2, header.diskno, header.diskcount);

    swprintf(item->strTitleID, L"%08X", header.mediaid);
	//item->strTitleID = sprintfa(L"%08x",header.titleid);
	//item = sprintfa(L"%08x",header.mediaid);
	//item->version = header.version1;
	//item->discno = header.diskno;
	//item->disccn = header.diskcount;

	fclose(fp);
}