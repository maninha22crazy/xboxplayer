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
#include "cc936.h"
#include "..\DeviceMgrLib\DeviceMgrLib.h"

GameList m_GameList;
int curSel = 0;
int curPage = 1;			// add:��ǰҳ date:2009-11-18 by:chengang
int pageSize = 0;			// add:ҳ��ʾ���� date:2009-11-18 by:chengang
int countPages = 1;			// add:��ҳ�� date:2009-11-18 by:chengang


char* m_curRoot = "";
bool m_IsFat = false;
UINT m_curDevice = DRIVE_DEVKIT;
CXuiControl m_lbDevice;

VOID LoadGameList(GameList *m_GameList);


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

/**--------------------------------------------------------------------------------------
 * Unicode2OEMCP - ת��Ϊ����Unicode 936
 * @Dest: ת������ַ���
 * @Source: ԭ�ַ���
 *
 * Returns����
 * Author��chengang
 * History��2009/12/22
 --------------------------------------------------------------------------------------*/
void Unicode2OEMCP(WCHAR* Dest,const CHAR* Source)
{
    int i = 0, j = 0;
    while(Source[j] != '\0')
    {
		if(Source[j] < 0)
		{
			if(m_IsFat)
			{
				const char* utf8 = (CHAR*)&(Source[j]);
				wchar_t unicode;
				unicode = (utf8[0] & 0x1F) << 12;
				unicode |= (utf8[1] & 0x3F) << 6;
				unicode |= (utf8[2] & 0x3F);

				Dest[i] = unicode;
				j +=3;
			}
			else
			{
				Dest[i] = ff_convert(*((WCHAR*)&(Source[j])),1);
				j +=2;
			}
		}
		else
		{
			Dest[i] = *((CHAR*)&(Source[j]));
			j ++;
		}
		++i;
    }
	Dest[i] = '\0';
}

bool MountDevice(UINT DriveType)
{
	// ����Lib֧�ֵ�6������
	DeviceMgrLib::MapExternalDrives();

	bool isOk = false;
	m_IsFat = false;
	m_curDevice = DriveType;
	switch(DriveType)
	{
		case DRIVE_USB0:
			isOk = DeviceMgrLib::IsMounted_USB0();
			m_curRoot = "Usb0";
			m_lbDevice.SetText(L"Usb0");
			m_IsFat = true;
			break;
		case DRIVE_USB1:
			isOk = DeviceMgrLib::IsMounted_USB1();
			m_curRoot = "Usb1";
			m_lbDevice.SetText(L"Usb1");
			m_IsFat = true;
			break;
		case DRIVE_USB2:
			isOk = DeviceMgrLib::IsMounted_USB2();
			m_curRoot = "Usb2";
			m_lbDevice.SetText(L"Usb2");
			m_IsFat = true;
			break;
		case DRIVE_DVD:
			isOk = DeviceMgrLib::IsMounted_DVD();
			m_curRoot = "Dvd";
			m_lbDevice.SetText(L"Dvd");
			break;
		case DRIVE_FLASH:
			isOk = DeviceMgrLib::IsMounted_FLASH();
			m_curRoot = "Flash";
			m_lbDevice.SetText(L"Flash");
			break;
		case DRIVE_HDD:
			isOk = DeviceMgrLib::IsMounted_HDD();
			m_curRoot = "Hdd";
			m_lbDevice.SetText(L"Hdd");
			break;
		case DRIVE_DEVKIT:
			isOk = DeviceMgrLib::IsMounted_HDD();
			m_curRoot = "devkit";
			m_lbDevice.SetText(L"devkit");
			isOk = isOk && (S_OK == DmMapDevkitDrive());
			break;
	}

	if(isOk)
	{
		LoadGameList(&m_GameList);
	}
	return isOk;
}

/**--------------------------------------------------------------------------------------
 * getGameTitle - ��Ϸ�ļ�˵��
 * @lpFileName: ��Ϸ˵���ļ�
 * @lpGameName: ��Ϸ��
 *
 *�ļ�Ŀ¼�ṹ��
 *				default.txt ��Ϸ�ļ�˵������������ڣ�ֱ��ʹ��Ŀ¼����
 * Returns���ɹ���־
 * Author��GooHome
 * History��2009/12/12 ��������
 --------------------------------------------------------------------------------------*/
bool getGameTitle(char* lpFileName,char* lpGameName)
{
		HANDLE hFile = CreateFile(lpFileName,
									GENERIC_READ,
									FILE_SHARE_READ, 
									NULL, OPEN_EXISTING,
									FILE_FLAG_SEQUENTIAL_SCAN, NULL );

		DWORD nBytesToRead=40;
		DWORD nBytesRead=40,dwError;
		char lpBuffer[50]="";//�ļ���ȡ������ 
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

/**--------------------------------------------------------------------------------------
 * LoadGameList - ��ǰĿ¼�µ�HiddenĿ¼�µĵ�һ��Ŀ¼���ض������б���
 * @m_GameList: ��Ϸ�б�����
 *
 *�ļ�Ŀ¼�ṹ��default.xex ִ���ļ�
 *				default.txt ��Ϸ�ļ�˵������������ڣ�ֱ��ʹ��Ŀ¼����
 *				default.png ��ϷͼƬ
 * Returns��û�з���ֵ��͵����
 * Author��GooHome
 * History��2009/12/12 ��������
 --------------------------------------------------------------------------------------*/
VOID LoadGameList(GameList *m_GameList)
{
    char strFind[MAX_PATH];
	memset(strFind, 0, MAX_PATH);
	sprintf(strFind, "%s:\\hidden\\*", m_curRoot);

	//sprintf(strFind, "%s", "devkit:\\a\\*");
	char lpNewNameBuf[MAX_PATH];
	char lpGameName[MAX_PATH];
    WIN32_FIND_DATA wfd;
    HANDLE hFind;

	//�����Ϸ�б�
	m_GameList->clear();
	int n = m_GameList->size();
	// ��ʼ���ҵ�һ��ƥ����ļ�
    hFind = FindFirstFile( strFind, &wfd );

    if( INVALID_HANDLE_VALUE != hFind )
    {
        // ѭ���ҵ����е��ļ�
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
					//��Ϸ�ڵ���Ϣ
					GameNode *pGlist;
					pGlist=(GameNode *)malloc( sizeof(GameNode));
					memset(pGlist, 0,  sizeof(GameNode));

					memset(lpNewNameBuf, 0, MAX_PATH);
					sprintf(lpNewNameBuf, "%s:\\hidden\\%s\\default.txt", m_curRoot,wfd.cFileName);
					//��Ϸ�����ļ����ڣ���ȡ��Ϣ
					//������ʾ����todo
					if(getGameTitle(lpNewNameBuf,lpGameName)){
						memset(lpNewNameBuf, 0, MAX_PATH);
						sprintf(lpNewNameBuf, "%s",lpGameName);
						mbstowcs(pGlist->strName,lpNewNameBuf,strlen(lpNewNameBuf));
					}
					else{
						// ֧������Unicode
						Unicode2OEMCP(pGlist->strName,wfd.cFileName);
					}

					//������ϷͼƬ
					memset(pGlist->strImg, 0, MAX_PATH);
					sprintf(pGlist->strImg, "file://%s:/hidden/%s/default.png", m_curRoot,wfd.cFileName);

					//������Ϸִ���ļ�
					memset(pGlist->strPath, 0, MAX_PATH);
					sprintf(pGlist->strPath, "%s:\\hidden\\%s\\default.xex", m_curRoot,wfd.cFileName);

					m_GameList->push_back(*pGlist);
					CloseHandle(hFile);
				}			
			}

        } while( FindNextFile( hFind, &wfd ) );

        //�رղ����ļ����
        FindClose( hFind );
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
		// edit:ֻ��һҳ������ date:2009-11-18 by:chengang
		int nCount = m_GameList.size() - pageSize * (curPage - 1);
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
			// edit:����ǰ��ҳ����ʾ�ĸ��� date:2009-11-18 by;chengang
			pGetSourceTextData->szText = m_GameList[pageSize * (curPage - 1) + pGetSourceTextData->iItem].strName;
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
		GetChildById( L"lbDevice", &m_lbDevice );

		// add:Ĭ�϶�ȡxdkӲ�� date:2009-12-23 by:chengang
		MountDevice(m_curDevice);
		//LoadGameList(&m_GameList);

		countPages = (m_GameList.size() * 1.0 / pageSize - m_GameList.size() / pageSize) > 0 ? ( m_GameList.size() / pageSize + 1) : ( m_GameList.size() / pageSize);
		swprintf(wszPageText, L"��ǰҳ�� 1/%d  [����%d]", countPages,m_GameList.size());
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
			// edit:����ǰ��ҳ����ʾ�ĸ��� date:2009-11-18 by;chengang
            curSel = pageSize * (curPage - 1) + m_List.GetCurSel();
			curSel = m_List.GetCurSel();
			m_Value.SetText( m_GameList[curSel].strName );


			// edit:����ͼƬ date:2009-12-23 by:chengang
			WCHAR lpImgPathBuf[MAX_PATH];
			memset(lpImgPathBuf, 0, MAX_PATH);
			mbstowcs(lpImgPathBuf,m_GameList[curSel].strImg,strlen(m_GameList[curSel].strImg));
			m_GameImage.SetImagePath(lpImgPathBuf);

            bHandled = TRUE;
        }

        return S_OK;
    }

	HRESULT OnKeyDown(XUIMessageInput *pInputData,BOOL &bHandled)
	{
		bool isPage = false;
		bool isChangeDevice = false;
		switch ( pInputData->dwKeyCode )
        {
			// ���¼�����Ϸ�б�
            case VK_PAD_Y:
            {
				LoadGameList(&m_GameList);
                break;
            }
			// ����xna����
            case VK_PAD_BACK:
            {
				XLaunchNewImage( XLAUNCH_KEYWORD_DASH, 0 );
                break;
            }
			// ���ص�xdk����
            case VK_PAD_B:
            {
				XLaunchNewImage( XLAUNCH_KEYWORD_DEFAULT_APP, 0 );
                break;
            }
			// ��ҳ
			case VK_PAD_LSHOULDER:
            {
				if(curPage > 1)
				{
					curPage--;
					isPage = true;
				}
                break;
            }
			// ��ҳ
			case VK_PAD_RSHOULDER:
            {
				if(curPage < countPages)
				{
					curPage++;
					isPage = true;
				}
                break;
            }
			// ���л����豸��
			case VK_PAD_DPAD_LEFT:
			{
				m_curDevice = m_curDevice == DRIVE_DEVKIT ? DRIVE_HDD : m_curDevice - 1;
				MountDevice(m_curDevice);
				isChangeDevice = true;
				break;
			}
			// ���л����豸��
			case VK_PAD_DPAD_RIGHT:
			{
				m_curDevice = m_curDevice == DRIVE_HDD ? DRIVE_DEVKIT : m_curDevice + 1;
				MountDevice(m_curDevice);
				isChangeDevice = true;
				break;
			}
        }
        
		// add:�Ƿ��л���ҳ date:2009-11-18 by:chengang
		if(isPage)
		{
			swprintf(wszPageText, L"��ǰҳ�� %d/%d  [����%d]", curPage,countPages,m_GameList.size());
			m_Page.SetText(wszPageText);
			curSel = pageSize * (curPage - 1) + m_List.GetCurSel();
			if(curSel > m_GameList.size() - 1)
			{
				curSel = m_GameList.size() - 1;
				m_List.SetCurSel(curSel -  pageSize * (curPage - 1));
			}
			m_Value.SetText( m_GameList[curSel].strName );

			// edit:����ͼƬ date:2009-12-23 by:chengang
			//m_GameImage.SetImagePath(m_GameList[curSel].strImg);
			WCHAR lpImgPathBuf[MAX_PATH];
			memset(lpImgPathBuf, 0, MAX_PATH);
			mbstowcs(lpImgPathBuf,m_GameList[curSel].strImg,strlen(m_GameList[curSel].strImg));
			m_GameImage.SetImagePath(lpImgPathBuf);

			m_List.DeleteItems(0,m_List.GetItemCount());
		}
		else if(isChangeDevice)
		{
			curPage = 1;
			swprintf(wszPageText, L"��ǰҳ�� %d/%d  [����%d]", curPage,countPages,m_GameList.size());
			m_Page.SetText(wszPageText);
			m_List.DeleteItems(0,m_List.GetItemCount());
			curSel = 0;
			int nCount = m_GameList.size();
			if(nCount > 0)
			{
				m_Value.SetText( m_GameList[curSel].strName );

				// add:����ͼƬ date:2009-12-23 by:chengang
				WCHAR lpImgPathBuf[MAX_PATH];
				memset(lpImgPathBuf, 0, MAX_PATH);
				mbstowcs(lpImgPathBuf,m_GameList[curSel].strImg,strlen(m_GameList[curSel].strImg));
				m_GameImage.SetImagePath(lpImgPathBuf);
				nCount = nCount > pageSize ? pageSize : nCount;
				m_List.InsertItems(0,nCount);
				m_List.SetCurSel(curSel);
			}
			else
			{
				m_Value.SetText(L"");
				m_GameImage.SetImagePath(L"");
			}
		}
	}


	HRESULT OnNotifyPress( HXUIOBJ hObjPressed, BOOL& bHandled )
	{
		// edit:ʹ��SONIC3D��װ��api date:2009-12-23 by:chengang
		XLaunchNewImage( m_GameList[curSel].strPath, 0 );
		//DeviceMgrLib::LaunchExternalImage(m_GameList[curSel].strPath,0);   // �޷����У�
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

    // ��ʼ����Ϸ�б�
    //LoadGameList();

    // Load the skin file used for the scene.
    app.LoadSkin( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\simple_scene_skin.xur" );

    // Load the scene.
	XVIDEO_MODE VideoMode; 
	XMemSet( &VideoMode, 0, sizeof(XVIDEO_MODE) ); 
	XGetVideoMode( &VideoMode );

	// ��ͬ�ֱ���ѡ��ͬ�Ľ���
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

