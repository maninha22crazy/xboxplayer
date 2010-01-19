/*=====================================================================//
//	Name:		Utility.h
//	Desc:		���ýṹ�塢�����Ķ���
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
/*							     usb�豸ID����                          */
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
	int   nOemCode;				// ���룺0-ʹ��ϵͳ;932-ShiftJIS(��);936(GBK����);949(��);950(Big5����)
	int   nLanguage;			// ���ػ����ԣ�0-ʹ��ϵͳ;1-Ӣ��;2-����;7-����;8-����(��);10-����(����);����ֵ-Ӣ��
	WCHAR* strDevice;			// ���ѡ�е�������
	int   nShowWall;			// �Ƿ���ʾ����ͼ��Ĭ�ϴ�-1;0-�ر�
	int   nShowNewWall;			// �Ƿ���ʾ�Զ��屳��ͼ��Ĭ�Ϸ�-0;1-��
	WCHAR* strWallPath;			// Ҫ��ʾ�ı���ͼ
};

//*========================================================================//
//	Name	:	CP_Init
//  Desc	:	��ʼ������
//	Param	: 
//			Dest:	��Ҫת�����ַ���
//
//	Return	:	�Ƿ��ʼ���ɹ�
//	Coder	:	EME
//	Date	:	2009-12-29
//========================================================================*/
BOOL CP_Init(WORD cp);




//*========================================================================//
//	Name	:	UnicodeToAnsi
//  Desc	:	Unicodeת��Ansi
//	Param	: 
//			Dest:	��Ҫת�����ַ���
//
//	Return	:	����ת������ַ���
//	Coder	:	EME
//	Date	:	2009-12-24
//========================================================================*/
LPSTR UnicodeToAnsi(LPCWSTR Dest);



//*========================================================================//
//	Name	:	ConvertFileName
//  Desc	:	ת���ļ����ı�������
//	Param	: 
//			Dest:	ת������ַ���
//			Source:	ԭ�ַ���
//			isUtf8:	�Ƿ�utf8��ʽ
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