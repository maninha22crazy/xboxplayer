
#include "Utility.h"
#include "cc936.h"


//--------------------------------------------------------------------------------------
// Name: UnicodeToAnsi
// Desc: Unicode转换Ansi
//--------------------------------------------------------------------------------------
LPSTR UnicodeToAnsi(LPCWSTR Dest)
{
	if (Dest == NULL) 
	{
		return NULL;
	}
	int cw = lstrlenW(Dest);
	if (cw==0) 
	{
		CHAR *psz = new CHAR[1];
		*psz = '\0';
		return psz;
	}
	int cc = WideCharToMultiByte(CP_ACP, 0, Dest, cw, NULL, 0, NULL, NULL );
	if (cc == 0)
	{
		return NULL;
	}
	CHAR *psz = new CHAR[cc + 1];
	cc = WideCharToMultiByte(CP_ACP ,0, Dest, cw, psz, cc, NULL, NULL );
	if (cc == 0) 
	{
		delete[] psz;
		return NULL;
	}
	psz[cc] = '\0';
	return psz;
}


//--------------------------------------------------------------------------------------
// Name: ConvertFileName
// Desc: 转换文件名的编码问题
//--------------------------------------------------------------------------------------
void ConvertFileName(WCHAR* Dest,const CHAR* Source,bool isUtf8)
{
    int i = 0, j = 0;
    while(Source[j] != '\0')
    {
		if(Source[j] < 0)
		{
			// fat格式时，原文件名为utf8格式
			if(isUtf8)
			{
				const char* utf8 = (CHAR*)&(Source[j]);
				wchar_t unicode;
				unicode = (utf8[0] & 0x1F) << 12;
				unicode |= (utf8[1] & 0x3F) << 6;
				unicode |= (utf8[2] & 0x3F);

				Dest[i] = unicode;
				j +=3;
			}
			else
			{
				// 转换为中文Unicode 936
				Dest[i] = ff_convert(*((WCHAR*)&(Source[j])),1);
				j +=2;
			}
		}
		else
		{
			Dest[i] = *((CHAR*)&(Source[j]));
			j ++;
		}
		++i;
    }
	Dest[i] = '\0';
}