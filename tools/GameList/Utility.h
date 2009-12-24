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


struct GameNode
{
    WCHAR   strName[MAX_PATH];
    CHAR   strPath[MAX_PATH];
	CHAR   strImg[MAX_PATH];
	FILETIME ftCreationTime;
};
typedef std::vector <GameNode> GameList;


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

#endif