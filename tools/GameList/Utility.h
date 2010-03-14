/*=====================================================================//
//	Name:		Utility.h
//	Desc:		常用结构体、函数的定义
//	Coder:		EME
//	Date:		2009-12-23
//=====================================================================*/


#ifndef Utility_H
#define Utility_H

#include "..\DeviceMgrLib\DeviceMgrLib.h"
#include <string>
#include <vector>

using namespace std;

union uDate
{   
  UINT   limit_data;   
  unsigned   char   buffer[4];   
};  


struct DownNode
{
    CHAR	strLocalPath[MAX_PATH];
    CHAR	strRemotePath[MAX_PATH];
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
extern BOOL CP_Init(WORD cp);




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
extern void ConvertFileName(WCHAR* Dest,const CHAR* Source,bool isUtf8);


extern LPWSTR BuildPath(LPCWSTR s1,LPCWSTR s2,LPCWSTR s3);

extern LPWSTR StrAdd(LPCWSTR s1,LPCWSTR s2);

extern UINT ReadUInt32(CHAR* buff);

extern VOID RefashDevice();

extern string& replace_all(string& str,const string& old_value,const string& new_value) ;

extern int FileExistsA(const CHAR* file);

extern bool ExtInList(DownNode* ext, vector<DownNode>* filetypes);
#endif