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

#include <vector>
#include "Utility.h"
#include "GameList.h"
#include "..\DeviceMgrLib\DeviceMgrLib.h"
#include <algorithm>
#include "Glbs.h"
#include "AtgXmlFileParser.h"

using namespace std; 

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
VOID SortList(GameList *m_GameList, UINT SortType)
{
	if(m_GameList->size() == 0)
	{
		return;
	}
	if(SortType == 0)
	{
		if(m_bSortLess)
		{
			sort(m_GameList->begin(), m_GameList->end(),lessCreateTime);
		}
		else
		{
			sort(m_GameList->begin(), m_GameList->end(),greaterGameName);
		}
	}
	else if(SortType == 1)
	{
		if(m_bSortLess)
		{
			sort(m_GameList->begin(), m_GameList->end(),lessGameName);
		}
		else
		{
			sort(m_GameList->begin(), m_GameList->end(),greaterGameName);
		}
	}
	m_bSortLess = !m_bSortLess;
}

//--------------------------------------------------------------------------------------
// Name: LoadConfig
// Desc: 读取配置文件GameList.xml
//--------------------------------------------------------------------------------------
VOID LoadConfig(VOID)
{
	ATG::XMLParser parser;
	ATG::XmlFileParser xmlFile;
    parser.RegisterSAXCallbackInterface( &xmlFile );
    HRESULT hr = parser.ParseXMLFile( "game:\\GameList.xml" );

    if( SUCCEEDED( hr ) )
    {
	}
}
//--------------------------------------------------------------------------------------
// Name: MountDevice
// Desc: 挂载设备
//--------------------------------------------------------------------------------------
BOOL MountDevice(UINT DriveType)
{
	// 挂载Lib支持的6个分区
	DeviceMgrLib::MapExternalDrives();

	bool isOk = false;
	m_IsUtf8 = false;
	m_nCurDevice = DriveType;
	switch(DriveType)
	{
		case IDS_DRIVE_USB0:
			isOk = DeviceMgrLib::IsMounted_USB0();
			m_curRoot = "Usb0";
			m_lbDevice.SetText(L"[Usb0]");
			m_IsUtf8 = true;
			break;
		case IDS_DRIVE_USB1:
			isOk = DeviceMgrLib::IsMounted_USB1();
			m_curRoot = "Usb1";
			m_lbDevice.SetText(L"[Usb1]");
			m_IsUtf8 = true;
			break;
		case IDS_DRIVE_USB2:
			isOk = DeviceMgrLib::IsMounted_USB2();
			m_curRoot = "Usb2";
			m_lbDevice.SetText(L"[Usb2]");
			m_IsUtf8 = true;
			break;
		case IDS_DRIVE_DVD:
			isOk = DeviceMgrLib::IsMounted_DVD();
			m_curRoot = "Dvd";
			m_lbDevice.SetText(L"[Dvd]");
			break;
		case IDS_DRIVE_FLASH:
			isOk = DeviceMgrLib::IsMounted_FLASH();
			m_curRoot = "Flash";
			m_lbDevice.SetText(L"[Flash]");
			break;
		case IDS_DRIVE_HDD:
			isOk = DeviceMgrLib::IsMounted_HDD();
			m_curRoot = "Hdd";
			m_lbDevice.SetText(L"[Hdd]");
			break;
		case IDS_DRIVE_DEVKIT:
			isOk = DeviceMgrLib::IsMounted_HDD();
			m_curRoot = "Devkit";
			m_lbDevice.SetText(L"[Devkit]");
			//isOk = isOk && (S_OK == DmMapDevkitDrive());
			break;
	}

	if(isOk)
	{
		LoadGameList(&m_GameList);
	}
	return isOk;
}


//--------------------------------------------------------------------------------------
// Name: getGameTitle
// Desc: 游戏文件说明
//--------------------------------------------------------------------------------------
bool getGameTitle(char* lpFileName,char* lpGameName)
{
		HANDLE hFile = CreateFile(lpFileName,
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
// Name: LoadGameList
// Desc: 当前目录下的Hidden目录下的第一层目录加载都向量列表里
//--------------------------------------------------------------------------------------
VOID LoadGameList(GameList *m_GameList)
{
    char strFind[MAX_PATH];
	memset(strFind, 0, MAX_PATH);
	sprintf(strFind, "%s:\\hidden\\*", m_curRoot);

	char lpNewNameBuf[MAX_PATH];
	char lpGameName[MAX_PATH];
    WIN32_FIND_DATA wfd;
    HANDLE hFind;

	// 清空游戏列表
	m_GameList->clear();
	int n = m_GameList->size();
	// 开始查找第一个匹配的文件
    hFind = FindFirstFile( strFind, &wfd );

    if( INVALID_HANDLE_VALUE != hFind )
    {
        // 循环找到所有的文件
        do
        {
			if(FILE_ATTRIBUTE_DIRECTORY==wfd.dwFileAttributes){
				memset(lpNewNameBuf, 0, MAX_PATH);
				sprintf(lpNewNameBuf, "%s:\\hidden\\%s\\default.xex", m_curRoot,wfd.cFileName);
				HANDLE hFile = CreateFile(lpNewNameBuf,    // file to open
								   GENERIC_READ,           // open for reading
								   FILE_SHARE_READ,        // share for reading
								   NULL,                   // default security
								   OPEN_EXISTING,          // existing file only
								   FILE_FLAG_OVERLAPPED,   // overlapped operation
								   NULL);                  // no attr. template
			 
				if (hFile != INVALID_HANDLE_VALUE)
				{
					// 游戏节点信息
					GameNode *pGlist;
					pGlist=(GameNode *)malloc( sizeof(GameNode));
					memset(pGlist, 0,  sizeof(GameNode));

					memset(lpNewNameBuf, 0, MAX_PATH);
					sprintf(lpNewNameBuf, "%s:\\hidden\\%s\\default.txt", m_curRoot,wfd.cFileName);
					// 游戏描述文件存在，读取信息
					if(getGameTitle(lpNewNameBuf,lpGameName))
					{
						memset(lpNewNameBuf, 0, MAX_PATH);
						sprintf(lpNewNameBuf, "%s",lpGameName);
						mbstowcs(pGlist->strName,lpNewNameBuf,strlen(lpNewNameBuf));
					}
					else
					{
						ConvertFileName(pGlist->strName,wfd.cFileName,m_IsUtf8);
					}
					
					// 设置创建时间
					pGlist->ftCreationTime = wfd.ftCreationTime;

					// 设置游戏图片
					memset(pGlist->strImg, 0, MAX_PATH);
					sprintf(pGlist->strImg, "file://%s:/hidden/%s/default.png", m_curRoot,wfd.cFileName);

					// 设置游戏执行文件
					memset(pGlist->strPath, 0, MAX_PATH);
					sprintf(pGlist->strPath, "%s:\\hidden\\%s\\default.xex", m_curRoot,wfd.cFileName);

					m_GameList->push_back(*pGlist);
					CloseHandle(hFile);
				}			
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
			// edit:增加前面页数显示的个数 date:2009-11-18 by;EME
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
	CXuiControl m_Page;
    CXuiList m_List;
	CXuiImageElement m_GameImage;
	CXuiImageElement m_imgLogo;
	WCHAR  wszPageText[100];


    // Message map. Here we tie messages to message handlers.
    XUI_BEGIN_MSG_MAP()
        XUI_ON_XM_INIT( OnInit )
		XUI_ON_XM_KEYDOWN( OnKeyDown ) 
        XUI_ON_XM_NOTIFY_SELCHANGED( OnNotifySelChanged )
		XUI_ON_XM_NOTIFY_PRESS( OnNotifyPress )
    XUI_END_MSG_MAP()


    //----------------------------------------------------------------------------------
    // Performs initialization tasks - retrieves controls.
    //----------------------------------------------------------------------------------
    HRESULT OnInit( XUIMessageInit* pInitData, BOOL& bHandled )
    {
        // Retrieve controls for later use.
        GetChildById( L"labelValue", &m_Value );
        GetChildById( L"listGames", &m_List );
		GetChildById( L"GameImage", &m_GameImage );
		GetChildById( L"labelPage", &m_Page );
		GetChildById( L"lbDevice", &m_lbDevice );

		// add:默认读取xdk硬盘 date:2009-12-23 by:EME
		MountDevice(m_nCurDevice);
		m_nCountPage = (m_GameList.size() * 1.0 / m_nPageSize - m_GameList.size() / m_nPageSize) > 0 ? ( m_GameList.size() / m_nPageSize + 1) : ( m_GameList.size() / m_nPageSize);
		swprintf(wszPageText, L"当前页： 1/%d  [共：%d]", m_nCountPage,m_GameList.size());
		m_Page.SetText(wszPageText);

        return S_OK;
    }

    //----------------------------------------------------------------------------------
    // Updates the UI when the list selection changes.
    //----------------------------------------------------------------------------------
    HRESULT OnNotifySelChanged( HXUIOBJ hObjSource, XUINotifySelChanged* pNotifySelChangedData, BOOL& bHandled )
    {
        if( hObjSource == m_List )
        {
			// edit:增加前面页数显示的个数 date:2009-11-18 by;EME
			m_nCurSel = m_nPageSize * (m_nCurPage - 1) + m_List.GetCurSel();
			m_Value.SetText( m_GameList[m_nCurSel].strName );


			// edit:设置图片 date:2009-12-23 by:EME
			WCHAR lpImgPathBuf[MAX_PATH];
			memset(lpImgPathBuf, 0, MAX_PATH);
			mbstowcs(lpImgPathBuf,m_GameList[m_nCurSel].strImg,strlen(m_GameList[m_nCurSel].strImg));
			m_GameImage.SetImagePath(lpImgPathBuf);

            bHandled = TRUE;
        }

        return S_OK;
    }

	HRESULT OnNotifyPress( HXUIOBJ hObjPressed, BOOL& bHandled )
	{
		// edit:使用SONIC3D封装的api date:2009-12-23 by:EME
		//XLaunchNewImage( m_GameList[m_nCurSel].strPath, 0 );
		DeviceMgrLib::LaunchExternalImage(m_GameList[m_nCurSel].strPath,0);
		return S_OK;
	}

	HRESULT OnKeyDown(XUIMessageInput *pInputData,BOOL &bHandled)
	{
		bool isPage = false;
		bool isChangeDevice = false;
		switch ( pInputData->dwKeyCode )
        {
            case VK_PAD_Y:										// 重新加载游戏列表
            {
				LoadGameList(&m_GameList);
				isChangeDevice = true;						// 重新加载游戏列表跟切换设备一样的处理
                break;
            }
			//case VK_PAD_START:									// 返回到DASH界面
   //         {
			//	XLaunchNewImage( XLAUNCH_KEYWORD_DASH, 0 );
   //             break;
   //         }
            case VK_PAD_BACK:									// 返回到xdk界面
            {
				XLaunchNewImage( XLAUNCH_KEYWORD_DEFAULT_APP, 0 );
                break;
            }
            case VK_PAD_B:										// 返回到DASH界面
            {
				XLaunchNewImage( XLAUNCH_KEYWORD_DASH, 0 );
                break;
            }
			case VK_PAD_LSHOULDER:								// 切换（设备）
            {
				m_nCurDevice = m_nCurDevice == IDS_DRIVE_USB2 ? IDS_DRIVE_DEVKIT : m_nCurDevice + 1;
				MountDevice(m_nCurDevice);
				isChangeDevice = true;
				break;
            }
			case VK_PAD_RSHOULDER:								// 留给XBLA
            {

            }
			case VK_PAD_DPAD_LEFT:								// 上页
			{
				if(m_nCurPage > 1)
				{
					m_nCurPage--;
					isPage = true;
				}
                break;
			}
			case VK_PAD_DPAD_RIGHT:								// 下页
			{
				if(m_nCurPage < m_nCountPage)
				{
					m_nCurPage++;
					isPage = true;
				}
                break;
			}
			case VK_PAD_X:										// 排序？
			{
				SortList(&m_GameList,1);
				isChangeDevice = true;		// 排序话跟切换设备一样的处理，重新刷新整个列表
				break;
			}
        }
        
		// add:是否切换了页 date:2009-11-18 by:EME
		if(isPage)
		{
			swprintf(wszPageText, L"当前页： %d/%d  [共：%d]", m_nCurPage,m_nCountPage,m_GameList.size());
			m_Page.SetText(wszPageText);
			m_nCurSel = m_nPageSize * (m_nCurPage - 1) + m_List.GetCurSel();
			if(m_nCurSel > m_GameList.size() - 1)
			{
				m_nCurSel = m_GameList.size() - 1;
				m_List.SetCurSel(m_nCurSel -  m_nPageSize * (m_nCurPage - 1));
			}
			m_Value.SetText( m_GameList[m_nCurSel].strName );

			// edit:设置图片 date:2009-12-23 by:EME
			WCHAR lpImgPathBuf[MAX_PATH];
			memset(lpImgPathBuf, 0, MAX_PATH);
			mbstowcs(lpImgPathBuf,m_GameList[m_nCurSel].strImg,strlen(m_GameList[m_nCurSel].strImg));
			m_GameImage.SetImagePath(lpImgPathBuf);

			m_List.DeleteItems(0,m_List.GetItemCount());
		}
		else if(isChangeDevice)
		{
			m_nCountPage = (m_GameList.size() * 1.0 / m_nPageSize - m_GameList.size() / m_nPageSize) > 0 ? ( m_GameList.size() / m_nPageSize + 1) : ( m_GameList.size() / m_nPageSize);
			m_nCurPage = 1;
			swprintf(wszPageText, L"当前页： %d/%d  [共：%d]", m_nCurPage,m_nCountPage,m_GameList.size());
			m_Page.SetText(wszPageText);
			m_List.DeleteItems(0,m_List.GetItemCount());
			m_nCurSel = 0;
			int nCount = m_GameList.size();
			if(nCount > 0)
			{
				m_Value.SetText( m_GameList[m_nCurSel].strName );

				// add:设置图片 date:2009-12-23 by:EME
				WCHAR lpImgPathBuf[MAX_PATH];
				memset(lpImgPathBuf, 0, MAX_PATH);
				mbstowcs(lpImgPathBuf,m_GameList[m_nCurSel].strImg,strlen(m_GameList[m_nCurSel].strImg));
				m_GameImage.SetImagePath(lpImgPathBuf);
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
		bHandled = true;
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
    // Declare an instance of the XUI framework application.
    CMyApp app;


    // 初始化程序   
    HRESULT hr = app.Init( XuiD3DXTextureLoader );
    if( FAILED( hr ) )
        ATG::FatalError( "Failed intializing application.\n" );

	// add:读取配置文件 date:2009-12-30 by:chengang
	LoadConfig();
	// add:初始化编码 date:2009-12-29 by:chengang
	CP_Init(m_ConfigNode.nLanguage);

    // 注册字体文件
    hr = app.RegisterDefaultTypeface( L"Arial Unicode MS", L"file://game:/media/xarialuni.ttf" );
    if( FAILED( hr ) )
        ATG::FatalError( "Failed to register default typeface.\n" );

    // 载入所用的皮肤文件
    app.LoadSkin( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\simple_scene_skin.xur" );

    // 根据不同分辨率载入相应的场景文件.
	XVIDEO_MODE VideoMode; 
	XMemSet( &VideoMode, 0, sizeof(XVIDEO_MODE) ); 
	XGetVideoMode( &VideoMode );

	if(VideoMode.dwDisplayHeight < 720)
	{
		m_nPageSize = 10;
		app.LoadFirstScene( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\", L"XuiLocale_480.xur", NULL );
	}
	else if(VideoMode.dwDisplayHeight < 1080)
	{
		m_nPageSize = 12;
		app.LoadFirstScene( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\", L"XuiLocale_720.xur", NULL );
	}
	else
	{
		m_nPageSize = 20;
		app.LoadFirstScene( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\", L"XuiLocale_1080.xur", NULL );
	}

    app.Run();
    app.Uninit();
}