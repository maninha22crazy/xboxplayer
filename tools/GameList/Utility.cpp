
#include "Utility.h"
#include "cp932.h"
#include "cp936.h"
#include "cp949.h"
#include "cp950.h"
#include "Externs.h"



typedef WCHAR (*ff_convert) (WCHAR,UINT);
typedef WCHAR (*ff_wtoupper) (WCHAR);

struct iconv_table {
	        const char		*name;
	        WORD			codepage;
	        ff_convert		convert;
			ff_wtoupper		wtoupper;
	      };

static const struct iconv_table mappings[] = {
	        { "CP932",  932,  CP932_ff_convert,  CP932_ff_wtoupper  },
			{ "CP936",  936,  CP936_ff_convert,  CP936_ff_wtoupper  },
			{ "CP949",  949,  CP949_ff_convert,  CP949_ff_wtoupper  },
			{ "CP950",  950,  CP950_ff_convert,  CP950_ff_wtoupper  },
			{ NULL,     0,    NULL,          NULL }
			};

static const struct iconv_table *curr_mapping = NULL;


BOOL CP_Init(WORD cp)
{
  int i;
  bool isSeach = false;
  if (cp == 0)
     cp = 936;
  
  for (i = 0; i < sizeof(mappings); i++)
  {
      if (cp == mappings[i].codepage)
	  {
		 isSeach = true;
         break;
	  }
  }
  if(!isSeach)
  {
	  i = 1;	// 默认936，gbk编码 date:2009-12-30 by:EME
  }
  curr_mapping = mappings + i;
  return (TRUE);
}

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
				//Dest[i] = CP936_ff_convert(*((WCHAR*)&(Source[j])),1);
				if (!curr_mapping)
				{
			      CP_Init(936);
				}
				Dest[i] = (*curr_mapping->convert)(*((WCHAR*)&(Source[j])),1);
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


//--------------------------------------------------------------------------------------
// Name: BuildPath
// Desc: 连接
//--------------------------------------------------------------------------------------
LPWSTR BuildPath(LPCWSTR s1,LPCWSTR s2,LPCWSTR s3)
{
	int cw = lstrlenW(s1) + lstrlenW(s2) + lstrlenW(s3);
	WCHAR *psz = new WCHAR[cw + 1];
	int i = 0,j = 0;
	while(s1[j] != '\0')
    {
		psz[i++] = s1[j++];
    }
	j = 0;
	while(s2[j] != '\0')
    {
		psz[i++] = s2[j++];
    }
	j = 0;
	while(s3[j] != '\0')
    {
		psz[i++] = s3[j++];
    }
	psz[i] = '\0';
	return psz;
}

//--------------------------------------------------------------------------------------
// Name: StrAdd
// Desc: 连接
//--------------------------------------------------------------------------------------
LPWSTR StrAdd(LPCWSTR s1,LPCWSTR s2)
{
	int cw = lstrlenW(s1) + lstrlenW(s2);
	WCHAR *psz = new WCHAR[cw + 1];
	int i = 0,j = 0;
	while(s1[j] != '\0')
    {
		psz[i++] = s1[j++];
    }
	j = 0;
	while(s2[j] != '\0')
    {
		psz[i++] = s2[j++];
    }
	psz[i] = '\0';
	return psz;
}

UINT ReadUInt32(CHAR* buff)
{
	union   uDate   u1;   
	u1.buffer[0]   = buff[0];
	u1.buffer[1]   = buff[1];
	u1.buffer[2]   = buff[2];
	u1.buffer[3]   = buff[3];
	return u1.limit_data;
}

//----------------------------------------------------------------------------------
// 刷新设备列表(通过获取大小来判断)
//----------------------------------------------------------------------------------
VOID RefashDevice()
{
	int nCount = sizeof(m_DeviceMappings)/sizeof(m_DeviceMappings[0]);
    char szDir[MAX_PATH];
    // 显示当前设备的容量信息
    ULARGE_INTEGER FreeBytesAvailable;
    ULARGE_INTEGER TotalNumberOfBytes;
    ULARGE_INTEGER TotalNumberOfFreeBytes;

	for (int i = 0; i < nCount; i++)
	{
		//m_DeviceMappings[i].isSuccess = true;

        memset(szDir, 0, MAX_PATH);
        sprintf(szDir, "%s\\", UnicodeToAnsi(m_DeviceMappings[i].deviceNameW));
	    GetDiskFreeSpaceEx(szDir,&FreeBytesAvailable,&TotalNumberOfBytes,&TotalNumberOfFreeBytes);
        m_DeviceMappings[i].isSuccess = TotalNumberOfBytes.QuadPart > 0;

		/*switch(m_DeviceMappings[i].deviceIndex)
		{
			case IDS_DRIVE_USB0:
				m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_USB0();
				break;
			case IDS_DRIVE_USB1:
				m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_USB1();
				break;
			case IDS_DRIVE_USB2:
				m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_USB2();
				break;
			case IDS_DRIVE_DVD:
				m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_DVD();
				break;
			case IDS_DRIVE_FLASH:
				m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_FLASH();
				break;
			case IDS_DRIVE_HDD:
				m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_HDD();
				break;
			case IDS_DRIVE_DEVKIT:
				m_DeviceMappings[i].isSuccess = DeviceMgrLib::IsMounted_HDD();
				break;
		}*/
	}
}

string& replace_all(string& str,const string& old_value,const string& new_value) 
{ 
    while(true)
    { 
        string::size_type pos(0); 
        if( (pos=str.find(old_value))!=string::npos ) 
        str.replace(pos,old_value.length(),new_value); 
        else break; 
    } 
    return str; 
} 

int FileExistsA(const CHAR* file)
{
	return GetFileAttributes(file) != 0xFFFFFFFF;
}

bool ExtInList(DownNode* ext, vector<DownNode>* filetypes)
{
	if (!filetypes)
    {
		return true;
    }

	if (filetypes->size() == 0)
    {
        return true;
    }
	for (unsigned int i = 0 ; i < filetypes->size() ; i++)
	{
        if (strcmp(filetypes->at(i).strLocalPath, ext->strLocalPath) == 0)
        {
            return true;
        }
	}
	return false;
}