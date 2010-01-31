#ifndef Externs_H
#define Externs_H

#include <vector>

/************************************************************************/
/*							     usb设备ID定义                          */
/************************************************************************/
#define IDS_DRIVE_DEVKIT                       0
#define IDS_DRIVE_USB0							1
#define IDS_DRIVE_USB1							2
#define IDS_DRIVE_USB2							3
#define IDS_DRIVE_DVD							4
#define IDS_DRIVE_FLASH						5
#define IDS_DRIVE_HDD							6
#define IDS_DRIVE_HDDX							7

// 设备节点信息
struct device_table 
{
    char		*deviceName;
	const wchar_t		*deviceNameW;
    UINT			deviceIndex;
    bool		isSuccess;
	bool		isUtf8;
};

// 游戏节点信息
struct GameNode
{
    WCHAR   strName[MAX_PATH];
    CHAR	strPath[MAX_PATH];
	WCHAR   strTitleImagePath[MAX_PATH];
	FILETIME ftCreationTime;

	
	CHAR	strFileName[MAX_PATH];
	WCHAR   strWallPath[MAX_PATH];
	WCHAR	strIcoPath[MAX_PATH];
	WCHAR	strTitleID[0x40];
	WCHAR	strGameTitle[MAX_PATH];
	BOOL	bIsRegion;
};

// Arc节点信息（从xml中解析）
struct ArcadeInfo
{
	WCHAR strID[MAX_PATH];				// ID
	WCHAR strName[MAX_PATH];				// Name
	WCHAR strImagePath[MAX_PATH];		// ImagePath
	WCHAR strDescription[1024];		// Description
};

struct ConfigNode
{
	int   nOemCode;				// 编码：0-使用系统;932-ShiftJIS(日);936(GBK简体);949(韩);950(Big5繁体)
	int   nLanguage;			// 本地化语言：0-使用系统;1-英语;2-日语;7-韩语;8-中文(繁);10-中文(简体);其他值-英语
	WCHAR strDevice[MAX_PATH];			// 最后选中的驱动器
	int   nShowWall;			// 是否显示背景图：默认打开-1;0-关闭
	int   nShowNewWall;			// 是否显示自定义背景图：默认否-0;1-是
	WCHAR strWallPath[MAX_PATH];			// 要显示的背景图
	int   nGameType;			// 最后选中的游戏类别：0-游戏;1-xbla
};

typedef std::vector <GameNode> GameList;
extern struct device_table m_DeviceMappings[8];
extern LPCWSTR LocaleLanguage[11];						// 多语言支持
extern ConfigNode m_ConfigNode;						// 配置信息
extern ArcadeInfo m_ArcadeInfo;						// 当前选中的arc信息
extern XNADDR     m_xnaddr;

#endif