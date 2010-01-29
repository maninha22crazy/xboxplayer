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
	parser.AddAttribute("value",m_ConfigNode.strWallPath);
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
					if( !ReadFile( hFind1, m_pReadBuf, 0x1792, &NChars, NULL ))
					{
						CloseHandle(hFind1);
						return;
					}
					CloseHandle(hFind1) ;

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
		  }
      }
   }
   while (FindNextFile(hFind, &ffd) != 0);
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
	if( INVALID_HANDLE_VALUE != hFind )
	{
		do
		{
			if(FILE_ATTRIBUTE_DIRECTORY==wfd.dwFileAttributes){
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

					//==================================== 查找是否存在游戏背景图 begin ================================
					bool findAll = true;
					CHAR   strImg[MAX_PATH];
					memset(strImg, 0, MAX_PATH);
					sprintf(strImg, "%s\\hidden\\%s\\default_wall.png", m_curDevice.deviceName,wfd.cFileName);

					hFileFind = CreateFile( strImg, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );    
					if( hFileFind == INVALID_HANDLE_VALUE )
					{        
						findAll = false;
					}
					else
					{
						memset(strImg, 0, MAX_PATH);
						sprintf(strImg, "file://%s/hidden/%s/default_wall.png", m_curDevice.deviceName,wfd.cFileName);
						mbstowcs(pGlist->strWallPath,strImg,strlen(strImg));
					}
					CloseHandle(hFileFind);

					CHAR   strIco[MAX_PATH];
					memset(strIco, 0, MAX_PATH);
					sprintf(strIco, "%s\\hidden\\%s\\default_ico.png", m_curDevice.deviceName,wfd.cFileName);

					hFileFind = CreateFile( strIco, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );    
					if( hFileFind = INVALID_HANDLE_VALUE )
					{
						findAll = false;
					}
					else
					{
						memset(strIco, 0, MAX_PATH);
						sprintf(strIco, "file://%s/hidden/%s/default_ico.png", m_curDevice.deviceName,wfd.cFileName);
						mbstowcs(pGlist->strIcoPath,strIco,strlen(strIco));
					}
					CloseHandle(hFileFind);

					memset(pGlist->strPath, 0, MAX_PATH);
					sprintf(pGlist->strPath, "%s\\hidden\\%s\\default.xex", m_curDevice.deviceName,wfd.cFileName);

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
	


	#define NONE 0
	#define INFO 1
	UINT m_nManagerOption;


	// Message map. Here we tie messages to message handlers.
	XUI_BEGIN_MSG_MAP()
		XUI_ON_XM_INIT( OnInit )
		XUI_ON_XM_KEYDOWN( OnKeyDown ) 
		XUI_ON_XM_NOTIFY_SELCHANGED( OnNotifySelChanged )
		XUI_ON_XM_NOTIFY_PRESS( OnNotifyPress )
	XUI_END_MSG_MAP()


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
		// 挂载Lib支持的6个分区
		//DeviceMgrLib::MapExternalDrives();

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
		
		return isOk;
	}

	//--------------------------------------------------------------------------------------
	// Name: SetGameWall
	// Desc: 设置游戏背景图、读取ARC的信息
	//--------------------------------------------------------------------------------------
	VOID SetGameWall()
	{
		//m_labelArcName.SetText(L"");
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
			char mntpth[] = "dice";
			UINT32 ret;

			unmountCon(mntpth);
			ret = mountCon(mntpth, m_curDevice.deviceName, m_GameList[m_nCurSel].strPath);

			if(ret == 0)
			{
				// 解析xml，读取信息
				//ATG::XMLParser parser;
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
				else
				{
					m_GameIcoImage.SetImagePath(L"");
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

		//GetChildById( L"labelArcName", &m_labelArcName );
		GetChildById( L"labelIP", &m_labelIP );
		GetChildById( L"labelArcDescription", &m_labelArcDescription );



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
		//launchX( m_GameList[m_nCurSel].strPath, 0 );
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
			//DeviceMgrLib::LaunchExternalImage(m_GameList[m_nCurSel].strPath,0);
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
				case 5:				// 返回XDK界面
					SaveConfig();
					launchX( XLAUNCH_KEYWORD_DEFAULT_APP );
					break;
				case 6:				// 返回DASH界面
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

					//m_panelMenu.SetOpacity(1);
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
				//m_listMenu.SetOpacity(1);
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
// Name: main()
// Desc: 主函数，程序的入口处
//--------------------------------------------------------------------------------------
VOID __cdecl main()
{
	// 挂载全部的设备
	//DeviceMgrLib::MapExternalDrives();

	XNetStartupParams xnsp;
	memset(&xnsp, 0, sizeof(xnsp));
	xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
	xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;

	INT iResult = XNetStartup( &xnsp );

	if( iResult != NO_ERROR )
	   printf("XNETSTARTUP ERROR\n");

	// Start up Winsock
	WORD wVersion = MAKEWORD( 2, 2 );   // request version 2.2 of Winsock
	WSADATA wsaData;

	INT err = WSAStartup( wVersion, &wsaData );
	if( err != 0 )
	{
		ATG::FatalError( "WSAStartup failed, error %d.\n", err );
	}

	// Verify that we got the right version of Winsock
	if( wsaData.wVersion != wVersion )
	{
		ATG::FatalError( "Failed to get proper version of Winsock, got %d.%d.\n",LOBYTE( wsaData.wVersion ), HIBYTE( wsaData.wVersion ) );
	}

	DWORD   hddexist = (Mount( "Hdd","\\Device\\Harddisk0\\Partition1") == S_OK);
	hddexist = (Mount( "Devkit","\\Device\\Harddisk0\\Partition1\\Devkit") == S_OK);
	hddexist = (Mount( "Usb0","\\Device\\Mass0") == S_OK);
	hddexist = (Mount( "Usb1","\\Device\\Mass1") == S_OK);
	hddexist = (Mount( "Usb2","\\Device\\Mass2") == S_OK);
	hddexist = (Mount( "Dvd","\\Device\\Cdrom0") == S_OK);

	DWORD dwRet;
	do
	{
		dwRet = XNetGetTitleXnAddr( &m_xnaddr );
	} while( dwRet == XNET_GET_XNADDR_PENDING );

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


	// edit:仅支持720p分辨率 date:2010-01-13 by:EME
    // 根据不同分辨率载入相应的场景文件.
	//XVIDEO_MODE VideoMode; 
	//HXUIOBJ hScene;
	//XMemSet( &VideoMode, 0, sizeof(XVIDEO_MODE) ); 
	//XGetVideoMode( &VideoMode );
	
	//if(VideoMode.dwDisplayHeight < 720)
	//{
	//	m_nPageSize = 10;
	//	app.LoadFirstScene( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\", L"XuiLocale_480.xur", &hScene );
	//}
	//else if(VideoMode.dwDisplayHeight < 1080)
	//{
	//	m_nPageSize = 12;
	//	app.LoadFirstScene( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\", L"XuiLocale_720.xur", &hScene );
	//}
	//else
	//{
	//	m_nPageSize = 20;
	//	app.LoadFirstScene( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\", L"XuiLocale_1080.xur", &hScene );
	//}
	
	m_nPageSize = 13;
	app.LoadFirstScene( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\", L"XuiLocale_main.xur", NULL );

	app.Run();

	FtpServer.StopListening();
	app.Uninit();
}