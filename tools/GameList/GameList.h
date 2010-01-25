/*=====================================================================//
//	Name:		GameList.h
//	Desc:		头文件、全局变量、方法定义
//	Coder:		GooHome、EME
//	Date:		2009-12-23
//=====================================================================*/

#ifndef GameList_H
#define GameList_H

#include "Glbs.h"

#define BUFSIZE			5000000	
#define IDS_JP			932				// 日语Shift-JIS
#define IDS_CHS		936				// 中文简体
#define IDS_KOREAN		949				// 韩语
#define IDS_CHT		950				// 中文繁体Big5

GameList m_GameList;
INT m_nCurSel		= 0;				//	当前选中项
INT m_nCurPage		= 1;				//	当前选中页
INT m_nPageSize		= 0;				//	页的可显示项
INT m_nCountPage	= 1;				//	总页数
BOOL m_bSortLess	= false;			//	升降序

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


static const UINT BlockLevel[] = { 0xA8000, 0x154000 };
static const UINT nContentImageSize = 0x1716;			// 小图大小位置
static const UINT nContentImage = 0x571A;				// 小图位置

static const UINT nContentTitle = 0x1692;				// 标题位置
static	const UINT nContentTitleSize = 0x40;			// 标题宽度

static const UINT nTitleID = 0x360;					// TitleID位置

static const UINT nStart = 0xe000;						// 读取位置

WCHAR m_lpImgPathBuf[MAX_PATH];

/**--------------------------------------------------------------------------------------
 * LoadGameList - 当前目录下的Hidden目录下的第一层目录加载都向量列表里
 * @m_GameList: 游戏列表数组
 *
 *文件目录结构：default.xex 执行文件
 *				default.txt 游戏文件说明（如果不存在，直接使用目录名）
 *				default.png 游戏图片
 * Returns：没有返回值（偷懒）
 * Author：GooHome
 * History：2009/12/12 初版做成
 --------------------------------------------------------------------------------------*/
VOID LoadGameList();

VOID LoadXblaList();

VOID LoadList();

/**--------------------------------------------------------------------------------------
 * getGameTitle - 游戏文件说明
 * @lstrFileName: 游戏说明文件
 * @lpGameName: 游戏名
 *
 *文件目录结构：
 *				default.txt 游戏文件说明（如果不存在，直接使用目录名）
 * Returns：成功标志
 * Author：GooHome
 * History：2009/12/12 初版做成
 --------------------------------------------------------------------------------------*/
bool getGameTitle(char* lstrFileName,char* lpGameName);



//*========================================================================//
//	Name	:	SortList
//  Desc	:	对游戏列表进行排序
//	Param	: 
//			SortType:	排序的类型:
//										0	:	创建时间
//										1	:	其他...
//
//	Return	:	true - 挂载成功;flase - 挂载失败
//	Coder	:	EME
//	Date	:	2009-12-24
//========================================================================*/
VOID SortGameList(GameList *m_GameList, UINT SortType);

//*========================================================================//
//	Name	:	LoadConfig
//  Desc	:	读取配置文件
//	Param	: 
//	Return	:	没有返回值
//	Coder	:	EME
//	Date	:	2009-12-30
//========================================================================*/
VOID LoadConfig(VOID);

//*========================================================================//
//  Name	:	SaveConfig
//  Desc	:	保存配置文件
//	Param	: 
//	Return	:	没有返回值
//	Coder	:	EME
//	Date	:	2010-01-13
//========================================================================*/
VOID SaveConfig(VOID);

#endif