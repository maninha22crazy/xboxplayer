#include <xui.h>
#include "Utility.h"

#define IDS_JP			932				// ����Shift-JIS
#define IDS_CHS		936				// ���ļ���
#define IDS_KOREAN		949				// ����
#define IDS_CHT		950				// ���ķ���Big5

GameList m_GameList;
INT m_nCurSel		= 0;				//	��ǰѡ����
INT m_nCurPage		= 1;				//	��ǰѡ��ҳ
INT m_nPageSize		= 0;				//	ҳ�Ŀ���ʾ��
INT m_nCountPage	= 1;				//	��ҳ��


struct device_table m_DeviceMappings[] = {
			{ "Devkit",L"Devkit",  IDS_DRIVE_DEVKIT,  false,  false  },
	        { "Usb0",L"Usb0",   IDS_DRIVE_USB0,  false,	  true  },
			{ "Usb1",L"Usb1",   IDS_DRIVE_USB1,  false,	  true  },
			{ "Usb2",L"Usb2",   IDS_DRIVE_USB2,  false,	  true  },
			{ "Dvd", L"Dvd",  IDS_DRIVE_DVD,  false,	  true  }
			};

//BOOL m_IsUtf8		= false;			//	��ǰ�豸�Ƿ�utf8�����ļ���

CXuiTextElement m_lbGameTitle;	

BOOL m_bSortLess	= false;			//	������

struct device_table m_curDevice;
ConfigNode m_ConfigNode;				//  ������Ϣ

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

#define BUFSIZE			5000000	

 

static const UINT BlockLevel[] = { 0xA8000, 0x154000 };
static const UINT nContentImageSize = 0x1716;			// Сͼ��Сλ��
static const UINT nContentImage = 0x571A;				// Сͼλ��

static const UINT nContentTitle = 0x1692;				// ����λ��
static	const UINT nContentTitleSize = 0x40;			// ������

static const UINT nTitleID = 0x360;					// TitleIDλ��

static const UINT nStart = 0xe000;						// ��ȡλ��