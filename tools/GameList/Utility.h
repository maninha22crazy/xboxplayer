/*=====================================================================//
//	Name:		Utility.h
//	Desc:		常用结构体、函数的定义
//	Coder:		EME
//	Date:		2009-12-23
//=====================================================================*/


#ifndef Utility_H
#define Utility_H


//////////////////////////////////////////////////////////////////////////
//			Include Header File
#include <xtl.h>
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


struct device_table {
	        const char		*deviceName;
			const wchar_t		*deviceNameW;
	        UINT			deviceIndex;
	        bool		isSuccess;
			bool		isUtf8;
	      };

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
typedef std::vector <GameNode> GameList;

union uDate
{   
  UINT   limit_data;   
  unsigned   char   buffer[4];   
};  

struct ConfigNode
{
	int   nOemCode;				// 编码：0-使用系统;932-ShiftJIS(日);936(GBK简体);949(韩);950(Big5繁体)
	int   nLanguage;			// 本地化语言：0-使用系统;1-英语;2-日语;7-韩语;8-中文(繁);10-中文(简体);其他值-英语
	WCHAR* strDevice;			// 最后选中的驱动器
	int   nShowWall;			// 是否显示背景图：默认打开-1;0-关闭
	int   nShowNewWall;			// 是否显示自定义背景图：默认否-0;1-是
	WCHAR* strWallPath;			// 要显示的背景图
};

//*========================================================================//
//	Name	:	CP_Init
//  Desc	:	初始化编码
//	Param	: 
//			Dest:	需要转换的字符串
//
//	Return	:	是否初始化成功
//	Coder	:	EME
//	Date	:	2009-12-29
//========================================================================*/
BOOL CP_Init(WORD cp);




//*========================================================================//
//	Name	:	UnicodeToAnsi
//  Desc	:	Unicode转换Ansi
//	Param	: 
//			Dest:	需要转换的字符串
//
//	Return	:	返回转换后的字符串
//	Coder	:	EME
//	Date	:	2009-12-24
//========================================================================*/
LPSTR UnicodeToAnsi(LPCWSTR Dest);



//*========================================================================//
//	Name	:	ConvertFileName
//  Desc	:	转换文件名的编码问题
//	Param	: 
//			Dest:	转换后的字符串
//			Source:	原字符串
//			isUtf8:	是否utf8格式
//
//	Return	:	
//	Coder	:	EME
//	Date	:	2009-12-24
//========================================================================*/
void ConvertFileName(WCHAR* Dest,const CHAR* Source,bool isUtf8);


LPWSTR BuildPath(LPCWSTR s1,LPCWSTR s2,LPCWSTR s3);

LPWSTR StrAdd(LPCWSTR s1,LPCWSTR s2);

UINT ReadUInt32(CHAR* buff);
#endif