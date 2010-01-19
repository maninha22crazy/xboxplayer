#include <xui.h>
#include "Utility.h"

#define IDS_JP			932				// 日语Shift-JIS
#define IDS_CHS		936				// 中文简体
#define IDS_KOREAN		949				// 韩语
#define IDS_CHT		950				// 中文繁体Big5

GameList m_GameList;
INT m_nCurSel		= 0;				//	当前选中项
INT m_nCurPage		= 1;				//	当前选中页
INT m_nPageSize		= 0;				//	页的可显示项
INT m_nCountPage	= 1;				//	总页数


struct device_table m_DeviceMappings[] = {
			{ "Devkit",L"Devkit",  IDS_DRIVE_DEVKIT,  false,  false  },
	        { "Usb0",L"Usb0",   IDS_DRIVE_USB0,  false,	  true  },
			{ "Usb1",L"Usb1",   IDS_DRIVE_USB1,  false,	  true  },
			{ "Usb2",L"Usb2",   IDS_DRIVE_USB2,  false,	  true  },
			{ "Dvd", L"Dvd",  IDS_DRIVE_DVD,  false,	  true  }
			};

//BOOL m_IsUtf8		= false;			//	当前设备是否utf8命名文件名

CXuiTextElement m_lbGameTitle;	

BOOL m_bSortLess	= false;			//	升降序

struct device_table m_curDevice;
ConfigNode m_ConfigNode;				//  配置信息

CHAR* m_strConfigPath	= "game:\\GameList.xml";	// 配置文件位置
WCHAR* m_strAppWallPath	= L"file://game:/media/background.jpg";	// 背景图文件位置

WCHAR m_strGameList[256];					//	游戏列表标题
WCHAR m_strShowWall[256];					//	背景打开关闭菜单标题
WCHAR m_strShowNewWall[256];				//	设置背景打开关闭菜单标题

// 多语言支持
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
static const UINT nContentImageSize = 0x1716;			// 小图大小位置
static const UINT nContentImage = 0x571A;				// 小图位置

static const UINT nContentTitle = 0x1692;				// 标题位置
static	const UINT nContentTitleSize = 0x40;			// 标题宽度

static const UINT nTitleID = 0x360;					// TitleID位置

static const UINT nStart = 0xe000;						// 读取位置