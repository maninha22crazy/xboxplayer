/*=====================================================================//
//	Name:		GameList.h
//	Desc:		ͷ�ļ���ȫ�ֱ�������������
//	Coder:		GooHome��EME
//	Date:		2009-12-23
//=====================================================================*/

#ifndef GameList_H
#define GameList_H

#include "Glbs.h"

#define BUFSIZE			5000000	
#define IDS_JP			932				// ����Shift-JIS
#define IDS_CHS		936				// ���ļ���
#define IDS_KOREAN		949				// ����
#define IDS_CHT		950				// ���ķ���Big5

GameList m_GameList;
INT m_nCurSel		= 0;				//	��ǰѡ����
INT m_nCurPage		= 1;				//	��ǰѡ��ҳ
INT m_nPageSize		= 0;				//	ҳ�Ŀ���ʾ��
INT m_nCountPage	= 1;				//	��ҳ��
BOOL m_bSortLess	= false;			//	������

CHAR* m_strConfigPath	= "game:\\GameList.xml";	// �����ļ�λ��
WCHAR* m_strAppWallPath	= L"file://game:/media/background.jpg";	// ����ͼ�ļ�λ��

WCHAR m_strGameList[256];					//	��Ϸ�б����
WCHAR m_strShowWall[256];					//	�����򿪹رղ˵�����
WCHAR m_strShowNewWall[256];				//	���ñ����򿪹رղ˵�����

// ������֧��
LPCWSTR LocaleLanguage[11] =
{
	L"en-en",	// the default locale
	L"en-en",	// English
	L"ja-jp",	// Japanese
	L"de-de",	// German
	L"fr-fr",	// French
	L"es-es",	// Spanish
	L"it-it",	// Italian
	L"ko-kr",	// Korean
	L"zh-cht",	// Traditional Chinese
	L"pt-pt",	// Portuguese
	L"zh-chs"	// Simplified Chinese
};


static const UINT BlockLevel[] = { 0xA8000, 0x154000 };
static const UINT nContentImageSize = 0x1716;			// Сͼ��Сλ��
static const UINT nContentImage = 0x571A;				// Сͼλ��

static const UINT nContentTitle = 0x1692;				// ����λ��
static	const UINT nContentTitleSize = 0x40;			// ������

static const UINT nTitleID = 0x360;					// TitleIDλ��

static const UINT nStart = 0xe000;						// ��ȡλ��

WCHAR m_lpImgPathBuf[MAX_PATH];

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
VOID LoadGameList();

VOID LoadXblaList();

VOID LoadList();

/**--------------------------------------------------------------------------------------
 * getGameTitle - ��Ϸ�ļ�˵��
 * @lstrFileName: ��Ϸ˵���ļ�
 * @lpGameName: ��Ϸ��
 *
 *�ļ�Ŀ¼�ṹ��
 *				default.txt ��Ϸ�ļ�˵������������ڣ�ֱ��ʹ��Ŀ¼����
 * Returns���ɹ���־
 * Author��GooHome
 * History��2009/12/12 ��������
 --------------------------------------------------------------------------------------*/
bool getGameTitle(char* lstrFileName,char* lpGameName);



//*========================================================================//
//	Name	:	SortList
//  Desc	:	����Ϸ�б��������
//	Param	: 
//			SortType:	���������:
//										0	:	����ʱ��
//										1	:	����...
//
//	Return	:	true - ���سɹ�;flase - ����ʧ��
//	Coder	:	EME
//	Date	:	2009-12-24
//========================================================================*/
VOID SortGameList(GameList *m_GameList, UINT SortType);

//*========================================================================//
//	Name	:	LoadConfig
//  Desc	:	��ȡ�����ļ�
//	Param	: 
//	Return	:	û�з���ֵ
//	Coder	:	EME
//	Date	:	2009-12-30
//========================================================================*/
VOID LoadConfig(VOID);

//*========================================================================//
//  Name	:	SaveConfig
//  Desc	:	���������ļ�
//	Param	: 
//	Return	:	û�з���ֵ
//	Coder	:	EME
//	Date	:	2010-01-13
//========================================================================*/
VOID SaveConfig(VOID);

#endif