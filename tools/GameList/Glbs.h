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


//CHAR* m_curRoot		= "";				//	查找设备的根目录
BOOL m_IsUtf8		= false;			//	当前设备是否utf8命名文件名
//UINT m_nCurDevice	= IDS_DRIVE_DEVKIT;	//	当前选中的设备
CXuiControl m_lbDevice;					//	显示当前设备的label

BOOL m_bSortLess	= false;			//	升降序

CHAR* m_curRoot		= "";				//	查找设备的根目录
ConfigNode m_ConfigNode;				//  配置信息

CHAR* m_strConfigPath	= "game:\\GameList.xml";	// 配置文件位置

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