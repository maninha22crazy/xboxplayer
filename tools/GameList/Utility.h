/*=====================================================================//
//	Name:		Utility.h
//	Desc:		���ýṹ�塢�����Ķ���
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
//  Desc	:	��ʼ������
//	Param	: 
//			Dest:	��Ҫת�����ַ���
//
//	Return	:	�Ƿ��ʼ���ɹ�
//	Coder	:	EME
//	Date	:	2009-12-29
//========================================================================*/
extern BOOL CP_Init(WORD cp);




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
extern void ConvertFileName(WCHAR* Dest,const CHAR* Source,bool isUtf8);


extern LPWSTR BuildPath(LPCWSTR s1,LPCWSTR s2,LPCWSTR s3);

extern LPWSTR StrAdd(LPCWSTR s1,LPCWSTR s2);

extern UINT ReadUInt32(CHAR* buff);

extern VOID RefashDevice();

extern string& replace_all(string& str,const string& old_value,const string& new_value) ;

extern int FileExistsA(const CHAR* file);

extern bool ExtInList(DownNode* ext, vector<DownNode>* filetypes);
#endif