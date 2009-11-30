//--------------------------------------------------------------------------------------
// XuiLocale.cpp
//
// Shows how to implement a XUI based application supporting multiple locales.
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <xtl.h>
#include <xui.h>
#include <xuiapp.h>
#include <AtgUtil.h>


#include "AtgXmlFileParser.h"


ATG::XmlFileParser m_parser;
int curSel = 0;
int curPage = 1;			// add:当前页 date:2009-11-18 by:chengang
int pageSize = 0;			// add:页显示个数 date:2009-11-18 by:chengang
int countPages = 1;			// add:总页数 date:2009-11-18 by:chengang


LPSTR UnicodeToAnsi(LPCWSTR s)
{
	if (s==NULL) 
		return NULL;
	int cw=lstrlenW(s);
	if (cw==0) 
	{
		CHAR *psz=new CHAR[1];
		*psz='\0';
		return psz;
	}
	int cc = WideCharToMultiByte(CP_ACP,0,s,cw,NULL,0,NULL,NULL);
	if (cc==0)
		return NULL;
	CHAR *psz = new CHAR[cc+1];
	cc = WideCharToMultiByte(CP_ACP,0,s,cw,psz,cc,NULL,NULL);
	if (cc==0) 
	{
		delete[] psz;return NULL;
	}
	psz[cc]='\0';
	return psz;
}

void wtoc(CHAR* Dest, const WCHAR* Source)
{
    int i = 0;

    while(Source[i] != '\0')
    {
        Dest[i] = (CHAR)Source[i];
        ++i;
    }
	Dest[i] = '\0';
}

VOID LoadGameList(VOID)
{
	ATG::XMLParser parser;
    parser.RegisterSAXCallbackInterface( &m_parser );

    HRESULT hr = parser.ParseXMLFile( "game:\\GameList.xml" );

    if( SUCCEEDED( hr ) )
    {
	}
}
//--------------------------------------------------------------------------------------
// Name: class CGameList
// Desc: List implementation class.
//--------------------------------------------------------------------------------------
class CLanguageList : public CXuiListImpl
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
		// edit:只绑定一页的数据 date:2009-11-18 by:chengang
		int nCount = m_parser.m_GameList.size() - pageSize * (curPage - 1);
		pGetItemCountData->cItems = nCount > pageSize ? pageSize : nCount;
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
			// edit:增加前面页数显示的个数 date:2009-11-18 by;chengang
			pGetSourceTextData->szText = m_parser.m_GameList[pageSize * (curPage - 1) + pGetSourceTextData->iItem].strName;
			//pGetSourceTextData->szText = m_parser.m_GameList[pGetSourceTextData->iItem].strName;
            bHandled = TRUE;
        }
        return S_OK;
    }

public:

    // Define the class. The class name must match the ClassOverride property
    // set for the scene in the UI Authoring tool.
    XUI_IMPLEMENT_CLASS( CLanguageList, L"LanguageList", XUI_CLASS_LIST )
};


//--------------------------------------------------------------------------------------
// Name: class CMyMainScene
// Desc: Scene implementation class.
//--------------------------------------------------------------------------------------
class CMyMainScene : public CXuiSceneImpl
{
    // Control and Element wrapper objects.
    CXuiControl m_Value;
	CXuiControl m_Page;
    CXuiList m_List;
	CXuiImageElement m_GameImage;
	WCHAR  wszPageText[100];


    // Message map. Here we tie messages to message handlers.
    XUI_BEGIN_MSG_MAP()
        XUI_ON_XM_INIT( OnInit )
        XUI_ON_XM_NOTIFY_SELCHANGED( OnNotifySelChanged )
        XUI_ON_XM_NOTIFY_PRESS( OnNotifyPress )
		XUI_ON_XM_KEYDOWN( OnKeyDown )
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

		// add:页显示个数、总页数进行赋值 date:2009-11-18 by:chengang
		//pageSize = 12;//m_List.GetMaxLinesItemCount();	
		countPages = (m_parser.m_GameList.size() * 1.0 / pageSize - m_parser.m_GameList.size() / pageSize) > 0 ? ( m_parser.m_GameList.size() / pageSize + 1) : ( m_parser.m_GameList.size() / pageSize);
		swprintf(wszPageText, L"当前页： 1/%d  [共：%d]", countPages,m_parser.m_GameList.size());
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
			// edit:增加前面页数显示的个数 date:2009-11-18 by;chengang
            curSel = pageSize * (curPage - 1) + m_List.GetCurSel();
			//curSel = m_List.GetCurSel();
			m_Value.SetText( m_parser.m_GameList[curSel].strName );
			m_GameImage.SetImagePath(m_parser.m_GameList[curSel].strImg);
            bHandled = TRUE;
        }

        return S_OK;
    }

	HRESULT OnKeyDown(XUIMessageInput *pInputData,BOOL &bHandled)
	{
		bool isPage = false;
		switch ( pInputData->dwKeyCode )
        {
			// 进入xna界面
            case VK_PAD_BACK:
            {
				XLaunchNewImage( XLAUNCH_KEYWORD_DASH, 0 );
                break;
            }
			// 返回到xdk界面
            case VK_PAD_B:
            {
				XLaunchNewImage( XLAUNCH_KEYWORD_DEFAULT_APP, 0 );
                break;
            }
			// 上页
			case VK_PAD_LSHOULDER:
            {
				if(curPage > 1)
				{
					curPage--;
					isPage = true;
				}
                break;
            }
			// 下页
			case VK_PAD_RSHOULDER:
            {
				if(curPage < countPages)
				{
					curPage++;
					isPage = true;
				}
                break;
            }
        }
        
		// add:是否切换了页 date:2009-11-18 by:chengang
		if(isPage)
		{
			swprintf(wszPageText, L"当前页： %d/%d  [共：%d]", curPage,countPages,m_parser.m_GameList.size());
			m_Page.SetText(wszPageText);
			curSel = pageSize * (curPage - 1) + m_List.GetCurSel();
			if(curSel > m_parser.m_GameList.size() - 1)
			{
				curSel = m_parser.m_GameList.size() - 1;
				m_List.SetCurSel(curSel -  pageSize * (curPage - 1));
			}
			m_Value.SetText( m_parser.m_GameList[curSel].strName );
			m_GameImage.SetImagePath(m_parser.m_GameList[curSel].strImg);
			m_List.DeleteItems(0,m_List.GetItemCount());
		}
	}


	HRESULT OnNotifyPress( HXUIOBJ hObjPressed, BOOL& bHandled )
	{
		XLaunchNewImage( UnicodeToAnsi(m_parser.m_GameList[curSel].strPath), 0 );
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
// Desc: Registers all the scene classes.
//--------------------------------------------------------------------------------------
HRESULT CMyApp::RegisterXuiClasses()
{
    // Register any other classes necessary for the app/scene
    HRESULT hr = CMyMainScene::Register();
    if( FAILED( hr ) )
        return hr;

    hr = CLanguageList::Register();
    if( FAILED( hr ) )
        return hr;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: UnregisterXuiClasses()
// Desc: Unregisters all the scene classes.
//--------------------------------------------------------------------------------------
HRESULT CMyApp::UnregisterXuiClasses()
{
    CLanguageList::Unregister();
    CMyMainScene::Unregister();
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: main()
// Desc: Application entry point.
//--------------------------------------------------------------------------------------
VOID __cdecl main()
{
    // Declare an instance of the XUI framework application.
    CMyApp app;

    // Initialize the application.    
    HRESULT hr = app.Init( XuiD3DXTextureLoader );
    if( FAILED( hr ) )
        ATG::FatalError( "Failed intializing application.\n" );

    // Register a default typeface
    hr = app.RegisterDefaultTypeface( L"Arial Unicode MS", L"file://game:/media/xarialuni.ttf" );
    if( FAILED( hr ) )
        ATG::FatalError( "Failed to register default typeface.\n" );

    // 初始化游戏列表
    LoadGameList();

    // Load the skin file used for the scene.
    app.LoadSkin( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\simple_scene_skin.xur" );

    // Load the scene.
	XVIDEO_MODE VideoMode; 
	XMemSet( &VideoMode, 0, sizeof(XVIDEO_MODE) ); 
	XGetVideoMode( &VideoMode );

	// 不同分辨率选择不同的界面
	if(VideoMode.dwDisplayHeight <= 480)
	{
		pageSize = 10;
		app.LoadFirstScene( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\", L"XuiLocale_480.xur", NULL );
	}
	else if(VideoMode.dwDisplayHeight <= 720)
	{
		pageSize = 12;
		app.LoadFirstScene( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\", L"XuiLocale_720.xur", NULL );
	}
	else
	{
		pageSize = 20;
		app.LoadFirstScene( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\", L"XuiLocale_1080.xur", NULL );
	}

    app.Run();


    // Free resources, unregister custom classes, and exit.
    app.Uninit();
}

