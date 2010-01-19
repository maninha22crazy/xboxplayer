/*=====================================================================//
//	Name:		GameList.cpp
//	Desc:		������ڣ�����UI�����������
//	Coder:		GooHome��EME
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
#include "Glbs.h"
#include "AtgXmlFileParser.h"
#include "AtgXmlWriter.h"


using namespace std; 

//
//HANDLE g_hSetImageThread;
CXuiImageElement m_WallImage;
CXuiList m_List;
//UINT m_nCurImage = -1;
//
//
//DWORD WINAPI SetImageThreadProc(LPVOID lpParameter)
//{
//	while(1)
//	{
//		int n = m_nCurSel;
//		if(m_nCurImage != n)
//		{		
//			m_WallImage.SetImagePath(L"");
//			if(m_ConfigNode.nShowWall && n >= 0)
//			{
//				m_WallImage.SetImagePath(m_GameList[n].strWallPath);
//			}
//			m_nCurImage = n;
//		}
//		else
//		{
//			Sleep(100);
//		}
//	}
//}

#define  COMPARE_LENGTH 20  //�Ƚ��ַ�������Ϊ20�ֽ�
//--------------------------------------------------------------------------------------
// Name: lessGameName
// Desc: ����Ϸ�б���н�������
//--------------------------------------------------------------------------------------
bool lessGameName(const GameNode& s1,const GameNode& s2) 
{ 
	return wmemcmp(s1.strName ,s2.strName, COMPARE_LENGTH) > 0; 
}

//--------------------------------------------------------------------------------------
// Name: greaterGameName
// Desc: ����Ϸ�б������������
//--------------------------------------------------------------------------------------
bool greaterGameName(const GameNode& s1,const GameNode& s2) 
{ 
	return wmemcmp(s1.strName,s2.strName, COMPARE_LENGTH) < 0; 
} 

//--------------------------------------------------------------------------------------
// Name: lessCreateTime
// Desc: ����Ϸ�б�������ڽ�������
//--------------------------------------------------------------------------------------
bool lessCreateTime(const GameNode& s1,const GameNode& s2) 
{ 
	return CompareFileTime(&(s1.ftCreationTime), &(s2.ftCreationTime)) == 1; 
}

//--------------------------------------------------------------------------------------
// Name: greaterCreateTime
// Desc: ����Ϸ�б����������������
//--------------------------------------------------------------------------------------
bool greaterCreateTime(const GameNode& s1,const GameNode& s2) 
{ 
	return CompareFileTime(&(s1.ftCreationTime), &(s2.ftCreationTime)) == -1; 
} 

//--------------------------------------------------------------------------------------
// Name: SortList
// Desc: ����Ϸ�б��������
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
// Desc: ��ȡ�����ļ�GameList.xml
//--------------------------------------------------------------------------------------
VOID LoadConfig(VOID)
{
	ATG::XMLParser parser;
	ATG::XmlFileParser xmlFile;
	parser.RegisterSAXCallbackInterface( &xmlFile );
	HRESULT hr = parser.ParseXMLFile( m_strConfigPath );

	if( SUCCEEDED( hr ) )
	{
	}
}

//--------------------------------------------------------------------------------------
// Name: SaveConfig
// Desc: ���������ļ�GameList.xml
//--------------------------------------------------------------------------------------
VOID SaveConfig(VOID)
{
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

	parser.EndElement();
	parser.Close();
}


//--------------------------------------------------------------------------------------
// Name: getGameTitle
// Desc: ��Ϸ�ļ�˵��
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
// Name: LoadGameList
// Desc: ��ǰĿ¼�µ�HiddenĿ¼�µĵ�һ��Ŀ¼���ض������б���
//--------------------------------------------------------------------------------------
VOID LoadGameList(GameList *m_GameList)
{
	char strFind[MAX_PATH];
	memset(strFind, 0, MAX_PATH);
	sprintf(strFind, "%s:\\hidden\\*", m_curDevice.deviceName);

	char lpNewNameBuf[MAX_PATH];
	char lpGameName[MAX_PATH];
	WIN32_FIND_DATA wfd;
	HANDLE hFind;

	// �����Ϸ�б�
	m_GameList->clear();
	hFind = FindFirstFile( strFind, &wfd );

	HANDLE hFileFind;
	if( INVALID_HANDLE_VALUE != hFind )
	{
		do
		{
			if(FILE_ATTRIBUTE_DIRECTORY==wfd.dwFileAttributes){
				memset(lpNewNameBuf, 0, MAX_PATH);
				sprintf(lpNewNameBuf, "%s:\\hidden\\%s\\default.xex", m_curDevice.deviceName,wfd.cFileName);
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
					sprintf(lpNewNameBuf, "%s:\\hidden\\%s\\default.txt", m_curDevice.deviceName,wfd.cFileName);
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
					sprintf(strTitleImg, "file://%s:/hidden/%s/default.png", m_curDevice.deviceName,wfd.cFileName);
					mbstowcs(pGlist->strTitleImagePath,strTitleImg,strlen(strTitleImg));

					sprintf(pGlist->strFileName, "%s", wfd.cFileName);

					//==================================== �����Ƿ������Ϸ����ͼ begin ================================
					bool findAll = true;
					CHAR   strImg[MAX_PATH];
					memset(strImg, 0, MAX_PATH);
					sprintf(strImg, "%s:\\hidden\\%s\\default_wall.png", m_curDevice.deviceName,wfd.cFileName);

					hFileFind = CreateFile( strImg, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );    
					if( hFileFind == INVALID_HANDLE_VALUE )
					{        
						findAll = false;
					}
					else
					{
						memset(strImg, 0, MAX_PATH);
						sprintf(strImg, "file://%s:/hidden/%s/default_wall.png", m_curDevice.deviceName,wfd.cFileName);
						mbstowcs(pGlist->strWallPath,strImg,strlen(strImg));
					}

					CHAR   strIco[MAX_PATH];
					memset(strIco, 0, MAX_PATH);
					sprintf(strIco, "%s:/hidden/%s/default_ico.png", m_curDevice.deviceName,wfd.cFileName);

					hFileFind = CreateFile( strIco, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );    
					if( hFileFind = INVALID_HANDLE_VALUE )
					{
						findAll = false;
					}
					else
					{
						memset(strIco, 0, MAX_PATH);
						sprintf(strIco, "file://%s:/hidden/%s/default_ico.png", m_curDevice.deviceName,wfd.cFileName);
						mbstowcs(pGlist->strIcoPath,strIco,strlen(strIco));
					}

					memset(pGlist->strPath, 0, MAX_PATH);
					sprintf(pGlist->strPath, "%s:\\hidden\\%s\\default.xex", m_curDevice.deviceName,wfd.cFileName);

					pGlist->bIsRegion = findAll;
					//==================================== �����Ƿ������Ϸ����ͼ end ================================

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
// Desc: List�ؼ���ģ����
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
		// edit:ֻ��һҳ������ date:2009-11-18 by:EME
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
// Desc: ����ģ����
//--------------------------------------------------------------------------------------
class CMyMainScene : public CXuiSceneImpl
{
	// Control and Element wrapper objects.
	CXuiTextElement m_Value;
	CXuiTextElement m_Page;
	CXuiList m_listDevice;
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
	// Desc: ˢ�·�ҳ��Ϣ
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
	// Desc: �����豸
	//--------------------------------------------------------------------------------------
	BOOL MountDevice(WCHAR* deviceNameW)
	{
		bool isOk = false;
		// ����Lib֧�ֵ�6������
		DeviceMgrLib::MapExternalDrives();

		WCHAR   strTxt[MAX_PATH];
		memset(strTxt,0,MAX_PATH); 
		wcsncpy_s( strTxt,m_strGameList,wcslen(m_strGameList));
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

		wcsncat_s(strTxt,m_curDevice.deviceNameW,wcslen(m_curDevice.deviceNameW));
		m_lbGameTitle.SetText(strTxt);

		LoadGameList(&m_GameList);
		SortList(&m_GameList,1);		// ����
		RefreshPageInfo();
		
		return isOk;
	}

	//----------------------------------------------------------------------------------
	// ������ϷͼƬ
	//----------------------------------------------------------------------------------
	//VOID SetGameWall()
	//{
		//if(!m_GameList[m_nCurSel].bIsRegion)
		//{
		//	m_GameList[m_nCurSel].bIsRegion = true;
		//	LPWSTR lpGameNameW = new wchar_t[MAX_PATH];   //Nombre del fichero caracteres anchos

		//	string sNxeartFile;
		//	char cNxeartFile[MAX_PATH];

		//	sprintf(cNxeartFile, "%s:\\hidden\\%s\\nxeart", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName);


		//	ifstream in;
		//	stringstream out;
		//	string content;
		//	size_t start, end, length;

		//	in.open(cNxeartFile, ios::in | ios::binary);
		//	if(in.good())
		//	{
		//		CHAR startCode = 0xFFD8;
		//		CHAR endCode = 0xFFD9;

		//		out << in.rdbuf();
		//		content=out.str();

		//		string gameName = content.substr( 5778, 40 );
		//		for (size_t i = 0; i < gameName.length(); i++)
		//		{
		//			if (gameName.at(i) == 0x00)
		//				gameName.erase(i, 1);
		//		}
		//		gameName._Copy(m_GameList[m_nCurSel].strGameTitle, gameName.length(), gameName.length());
		//		m_GameList[m_nCurSel].strGameTitle[gameName.length()] = '/0';

		//		ofstream out2;
		//		if(m_GameList[m_nCurSel].strIcoPath[0] == 0)					// ��ȡСͼ
		//		{
		//			start = content.find("PNG");
		//			start -= 1;
		//			end = content.find("IEND");
		//			end += 7;
		//			length = end - start;
		//			string icon = content.substr( start, length );

		//			sprintf_s( m_GameList[m_nCurSel].strIcoPath,"%s:\\hidden\\%s\\default_ico.png", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName );
		//			out2.open(m_GameList[m_nCurSel].strIcoPath, ios::out | ios::binary);
		//			out2.write(icon.c_str(), icon.length());
		//			out2.close();
		//		}
		//		
		//		if(m_GameList[m_nCurSel].strWallPath[0] == 0)					// ��ȡ��ͼ
		//		{
		//			start = content.find("JFIF",start);
		//			if(start > 0)
		//			{
		//				end = content.find(endCode, start );
		//				start -= 6;
		//			}
		//			else
		//			{
		//				end = 2004881;
		//				start = 57344;
		//			}
		//			string wallpaperHD = content.substr( start, length );

		//			sprintf_s( m_GameList[m_nCurSel].strWallPath,"%s:\\hidden\\%s\\default.png", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName );
		//			out2.open(m_GameList[m_nCurSel].strWallPath, ios::out | ios::binary);
		//			out2.write(wallpaperHD.c_str(), wallpaperHD.length());
		//			out2.close();
		//		}
		//	}
		//}

		//memset(m_lpImgPathBuf, 0, MAX_PATH);
		//memset(m_GameList[m_nCurSel].strWallPath, 0, MAX_PATH);
		//sprintf(m_GameList[m_nCurSel].strWallPath, "file://%s:/hidden/%s/default.png", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName);
		//mbstowcs(m_lpImgPathBuf,m_GameList[m_nCurSel].strWallPath,strlen(m_GameList[m_nCurSel].strWallPath));
		//m_GameImage.SetImagePath(m_lpImgPathBuf);

		//memset(m_lpImgPathBuf, 0, MAX_PATH);
		//memset(m_GameList[m_nCurSel].strIcoPath, 0, MAX_PATH);
		//sprintf(m_GameList[m_nCurSel].strIcoPath, "file://%s:/hidden/%s/default_ico.png", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName);
		//mbstowcs(m_lpImgPathBuf,m_GameList[m_nCurSel].strIcoPath,strlen(m_GameList[m_nCurSel].strIcoPath));
		//m_List.SetImage(m_nCurSel,m_lpImgPathBuf);
	//}


	//----------------------------------------------------------------------------------
	// ������ϷͼƬ
	//----------------------------------------------------------------------------------
	VOID SetGameWall()
	{
		if(!m_GameList[m_nCurSel].bIsRegion)
		{
			m_GameList[m_nCurSel].bIsRegion = true;
			CHAR cNxeartFile[MAX_PATH];
			sprintf(cNxeartFile, "%s:\\hidden\\%s\\nxeart", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName);

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

				//================================== ��ȡID begin ==================================================
				ptr =  (CHAR *)m_pReadBuf + nTitleID;
				swprintf(m_GameList[m_nCurSel].strTitleID, L"%-8X", ReadUInt32(ptr));
				//================================== ��ȡID end ==================================================


				//================================== ��ȡ���� begin ==================================================
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
				//================================== ��ȡ���� end ==================================================


				//================================== ��ȡСͼ begin ==================================================
				if(m_GameList[m_nCurSel].strIcoPath[0] == 0)
				{
					ptr =  (CHAR *)m_pReadBuf + nContentImageSize;
					start_ptr = ptr;
					length = ReadUInt32(start_ptr);

					sprintf_s( strIcoPath,"%s:\\hidden\\%s\\default_ico.png", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName );
					out2.open(strIcoPath, ios::out | ios::binary);
					out2.write(start_ptr + 4,length);
					out2.close();
				}
				//================================== ��ȡСͼ end ==================================================


				//================================== ��ȡ��ͼ begin ==================================================
				if(m_GameList[m_nCurSel].strWallPath[0] == 0)
				{
					bool isJpeg = true;
					length = 0;

					ptr =  (CHAR *)m_pReadBuf + nStart;
					start_ptr = ptr;

					// �жϸ�ʽ(png��jpeg)
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
						sprintf_s(strWallPath,"%s:\\hidden\\%s\\default_wall.png", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName );
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
				//================================== ��ȡ��ͼ end ==================================================
			}

			sprintf(strIcoPath, "file://%s:/hidden/%s/default_ico.png", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName);
			mbstowcs(m_GameList[m_nCurSel].strIcoPath,strIcoPath,strlen(strIcoPath));

			sprintf(strWallPath, "file://%s:/hidden/%s/default_wall.png", m_curDevice.deviceName,m_GameList[m_nCurSel].strFileName);
			mbstowcs(m_GameList[m_nCurSel].strWallPath,strWallPath,strlen(strWallPath));
		}

		m_labelValueTitleId.SetText(m_GameList[m_nCurSel].strTitleID);
		m_labelValueTitleName.SetText(m_GameList[m_nCurSel].strGameTitle);

		// ���ñ���ͼ
		if(m_ConfigNode.nShowWall && !m_ConfigNode.nShowNewWall)
		{
			//m_WallImage.SetImagePath(L"");
			m_WallImage.SetImagePath(m_GameList[m_nCurSel].strWallPath);
			//m_nCurImage = m_nCurSel;
		}

		// ���÷���ͼ
		m_GameImage.SetImagePath(m_GameList[m_nCurSel].strTitleImagePath);

		// ����Сͼ
		m_GameIcoImage.SetImagePath(m_GameList[m_nCurSel].strIcoPath);
	}

	//----------------------------------------------------------------------------------
	// ˢ���豸�б�
	//----------------------------------------------------------------------------------
	VOID RefashDevice()
	{
		int nIndex = 0;
		m_listDevice.DeleteItems(0,m_listDevice.GetItemCount());
		int nCount = sizeof(m_DeviceMappings)/sizeof(m_DeviceMappings[0]);
		for (int i = 0; i < nCount; i++)
		{
			switch(m_DeviceMappings[i].deviceIndex)
			{
				case IDS_DRIVE_USB0:
					m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_USB0();
					break;
				case IDS_DRIVE_USB1:
					m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_USB1();
					break;
				case IDS_DRIVE_USB2:
					m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_USB2();
					break;
				case IDS_DRIVE_DVD:
					m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_DVD();
					break;
				case IDS_DRIVE_FLASH:
					m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_FLASH();
					break;
				case IDS_DRIVE_HDD:
					m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_HDD();
					break;
				case IDS_DRIVE_DEVKIT:
					m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_HDD();
					break;
			}
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
		GetChildById( L"listMenu", &m_listMenu );
		GetChildById( L"listGames", &m_List );
		GetChildById( L"listLanguage", &m_listLanguage );
		
		GetChildById( L"AppWallImage", &m_AppWallImage );
		GetChildById( L"WallImage", &m_WallImage );
		GetChildById( L"GameImage", &m_GameImage );
		GetChildById( L"GameIcoImage", &m_GameIcoImage );
		GetChildById( L"BottomImage", &m_BottomImage );

		
		GetChildById( L"labelPage", &m_Page );

		GetChildById( L"labelGameTitle", &m_lbGameTitle );

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
		

		LPCWSTR ss = m_lbGameTitle.GetText();
		wcsncpy_s( m_strGameList, ss, wcslen(ss) );

		ss = m_listMenu.GetText(2);
		wcsncpy_s( m_strShowWall, ss, wcslen(ss) );

		ss = m_listMenu.GetText(3);
		wcsncpy_s( m_strShowNewWall, ss, wcslen(ss) );
		
		if(m_ConfigNode.nShowWall)
		{
			m_listMenu.SetText(2,StrAdd(m_strShowWall,L"[OFF]"));
		}
		else
		{
			m_listMenu.SetText(2,StrAdd(m_strShowWall,L"[ON]"));
		}

		if(m_ConfigNode.nShowNewWall)
		{
			m_listMenu.SetText(3,StrAdd(m_strShowNewWall,L"[OFF]"));
		}
		else
		{
			m_listMenu.SetText(3,StrAdd(m_strShowNewWall,L"[ON]"));
		}


		// add:Ĭ�϶�ȡ�����е��豸 date:2009-12-23 by:EME
		MountDevice(m_ConfigNode.strDevice);

		m_nManagerOption = NONE;
		m_labelValueInfo.SetOpacity(0);
		m_panelMenu.SetOpacity(0);
		m_listMenu.SetOpacity(0);
		m_listDevice.SetOpacity(0);
		m_listLanguage.SetOpacity(0);
		m_List.SetOpacity(1);
		m_List.SetFocus(XUSER_INDEX_ANY);

		m_ImageB.SetOpacity(0);
		m_labelB.SetOpacity(0);

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
		// edit:ʹ��SONIC3D��װ��api date:2009-12-23 by:EME
		//XLaunchNewImage( m_GameList[m_nCurSel].strPath, 0 );
		if( hObjPressed == m_List )
		{
			bHandled = TRUE;
			DeviceMgrLib::LaunchExternalImage(m_GameList[m_nCurSel].strPath,0);
		}
		else if( hObjPressed == m_listDevice )
		{
			MountDevice((LPWSTR)(m_listDevice.GetText(m_listDevice.GetCurSel())));
		}
		else if( hObjPressed == m_listMenu )
		{
			switch(m_listMenu.GetCurSel())
			{
				case 0:				// ��ʾ�豸�б�
					m_listMenu.SetOpacity(0);
					RefashDevice();
					m_listDevice.SetOpacity(1);
					m_listDevice.SetFocus(XUSER_INDEX_ANY);
					break;
				case 1:
					m_listMenu.SetOpacity(0);
					m_listLanguage.SetOpacity(1);
					m_listLanguage.SetFocus(XUSER_INDEX_ANY);
					break;
				case 2:
					if(m_ConfigNode.nShowWall)
					{
						m_ConfigNode.nShowWall = 0;
						m_WallImage.SetImagePath(L"");
						m_listMenu.SetText(2,StrAdd(m_strShowWall,L"[ON]"));
					}
					else
					{
						m_ConfigNode.nShowWall = 1;
						if(m_List.GetCurSel() >= 0)
						{
							m_WallImage.SetImagePath(m_GameList[m_nCurSel].strWallPath);
						}
						m_listMenu.SetText(2,StrAdd(m_strShowWall,L"[OFF]"));
					}
					break;
				case 3:
					if(m_ConfigNode.nShowNewWall)
					{
						m_ConfigNode.nShowNewWall = 0;
						m_WallImage.SetImagePath(L"");
						m_listMenu.SetText(3,StrAdd(m_strShowNewWall,L"[ON]"));

						m_ConfigNode.strWallPath = L"";
						m_AppWallImage.SetImagePath(m_strAppWallPath);
					}
					else
					{
						m_ConfigNode.nShowNewWall = 1;
						if(m_List.GetCurSel() >= 0)
						{
							m_ConfigNode.strWallPath = m_GameList[m_nCurSel].strWallPath;
							m_AppWallImage.SetImagePath(m_ConfigNode.strWallPath);
						}
						m_listMenu.SetText(3,StrAdd(m_strShowNewWall,L"[OFF]"));
					}
					m_WallImage.SetImagePath(m_GameList[m_nCurSel].strWallPath);
					break;
				case 4:				// ����XDK����
					SaveConfig();
					XLaunchNewImage( XLAUNCH_KEYWORD_DEFAULT_APP, 0 );
					break;
				case 5:				// ����DASH����
					SaveConfig();
					XLaunchNewImage( XLAUNCH_KEYWORD_DASH, 0 );
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
					if(dwLanguage != 2 || dwLanguage != 7 || dwLanguage != 8 || dwLanguage != 10)
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
					dwLanguage = 7;
					break;
				case 4:	
					dwLanguage = 8;
					break;
				case 5:
					dwLanguage = 10;
					break;
			}

			if( dwLanguage>=sizeof( LocaleLanguage )/sizeof( LocaleLanguage[0] ) )
			{
				// ������Χ��ʹ��Ĭ������(Ӣ��)
				dwLanguage = 0;
			}
			m_ConfigNode.nLanguage = dwLanguage;
			XuiSetLocale( LocaleLanguage[dwLanguage] );
			XuiApplyLocale( m_hObj, NULL );
		}
		return S_OK;
	}

	HRESULT OnKeyDown(XUIMessageInput *pInputData,BOOL &bHandled)
	{
		CXuiControl menuItemCtrl;
		CXuiControl deviceItemCtrl;
		CXuiControl gameItemCtrl;
		CXuiControl languageItemCtrl;


		m_listMenu.GetCurSel(&menuItemCtrl);
		m_listDevice.GetCurSel(&deviceItemCtrl);
		m_List.GetCurSel(&gameItemCtrl);
		m_listLanguage.GetCurSel(&languageItemCtrl);

		bool isPage = false;
		if(m_nManagerOption == NONE)
		{
			if ( deviceItemCtrl == deviceItemCtrl.GetFocus(XUSER_INDEX_ANY))
			{
				// �������˵�
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
			else if ( menuItemCtrl == menuItemCtrl.GetFocus(XUSER_INDEX_ANY))
			{
				// ���ؽ���
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
				// �������˵�
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
				case VK_PAD_Y:										// �˵�
					m_ImageB.SetOpacity(1);
					m_labelB.SetOpacity(1);

					m_ImageY.SetOpacity(0);
					m_labelY.SetOpacity(0);

					m_panelMenu.SetOpacity(1);
					m_listMenu.SetOpacity(1);
					m_listMenu.SetFocus(XUSER_INDEX_ANY);
					break;
				case VK_PAD_LSHOULDER:								// ��ҳ
					if(m_nCurPage > 1)
					{
						m_nCurPage--;
						isPage = true;
					}
					break;
				case VK_PAD_RSHOULDER:								// ��ҳ
					if(m_nCurPage < m_nCountPage)
					{
						m_nCurPage++;
						isPage = true;
					}
					break;
				case VK_PAD_B:										// ��ʾ˵��
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

					// edit������ϷͼƬ date:2009-12-23 by:EME
					SetGameWall();
					m_List.DeleteItems(0,m_List.GetItemCount());
				}
			}
		}
		else if(m_nManagerOption == INFO)
		{
			// ���ؽ���
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
// Desc: ע��ȫ���ĳ�����
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
// Desc: ע��ʹ�õĳ���
//--------------------------------------------------------------------------------------
HRESULT CMyApp::UnregisterXuiClasses()
{
	CGameList::Unregister();
	CMyMainScene::Unregister();
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: main()
// Desc: ���������������ڴ�
//--------------------------------------------------------------------------------------
VOID __cdecl main()
{
	// Declare an instance of the XUI framework application.
	CMyApp app;

	HRESULT hr = app.Init( XuiD3DXTextureLoader );
	if( FAILED( hr ) )
		ATG::FatalError( "Failed intializing application.\n" );

	m_ConfigNode.nShowWall = 1;
	m_ConfigNode.nOemCode = 936; // Ĭ�ϼ���936����
	m_ConfigNode.strDevice = L"Devkit";
	m_ConfigNode.strWallPath = L"";
	// add:��ȡ�����ļ� date:2009-12-30 by:chengang
	LoadConfig();


	// ע�������ļ�
	hr = app.RegisterDefaultTypeface( L"Arial Unicode MS", L"file://game:/media/xarialuni.ttf" );
	if( FAILED( hr ) )
		ATG::FatalError( "Failed to register default typeface.\n" );

	// �������õ�Ƥ���ļ�
	app.LoadSkin( L"file://game:/media/XuiLocale.xzp#Media\\Xui\\simple_scene_skin.xur" );

	// ��ȡ��ǰ��ǰ����
	DWORD dwLanguage = XGetLanguage();

	if( dwLanguage>=sizeof( LocaleLanguage )/sizeof( LocaleLanguage[0] ) )
	{
		// ������Χ��ʹ��Ĭ������(Ӣ��)
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

	// Ĭ��Ӣ����ʾ
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

	// ����1�����õ�ǰXUI���ػ�
	XuiSetLocale( LocaleLanguage[dwLanguage] );

	// add:��ʼ������ date:2009-12-29 by:EME
	CP_Init(m_ConfigNode.nOemCode);


	// edit:��֧��720p�ֱ��� date:2010-01-13 by:EME
    // ���ݲ�ͬ�ֱ���������Ӧ�ĳ����ļ�.
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


	// ����2�����õ�ǰXUI���ػ�
	//HXUISTRINGTABLE hxuiStringtable;
	//LPWSTR pwstrPath;

	//pwstrPath = BuildPath( L"file://game:/media/", apwstrLocale[dwLanguage], L"/XuiLocale.xus" );
	//XuiLoadStringTableFromFile( pwstrPath, &hxuiStringtable );
	//XuiApplyLocale( hScene, hxuiStringtable );
	//XuiFreeStringTable( hxuiStringtable );

	//// ��ȡͼƬ�߳�
	//const DWORD STACK_SIZE = 0;
	//g_hSetImageThread = CreateThread( NULL, STACK_SIZE, SetImageThreadProc, NULL, 0, NULL );

	app.Run();
	app.Uninit();
}