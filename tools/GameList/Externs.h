#ifndef Externs_H
#define Externs_H

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

struct device_table 
{
    char		*deviceName;
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


struct ConfigNode
{
	int   nOemCode;				// ���룺0-ʹ��ϵͳ;932-ShiftJIS(��);936(GBK����);949(��);950(Big5����)
	int   nLanguage;			// ���ػ����ԣ�0-ʹ��ϵͳ;1-Ӣ��;2-����;7-����;8-����(��);10-����(����);����ֵ-Ӣ��
	WCHAR* strDevice;			// ���ѡ�е�������
	int   nShowWall;			// �Ƿ���ʾ����ͼ��Ĭ�ϴ�-1;0-�ر�
	int   nShowNewWall;			// �Ƿ���ʾ�Զ��屳��ͼ��Ĭ�Ϸ�-0;1-��
	WCHAR* strWallPath;			// Ҫ��ʾ�ı���ͼ
	int   nGameType;			// ���ѡ�е���Ϸ���0-��Ϸ;1-xbla
};

typedef std::vector <GameNode> GameList;
extern struct device_table m_DeviceMappings[7];
extern ConfigNode m_ConfigNode;						//  ������Ϣ
extern LPCWSTR LocaleLanguage[11];						// ������֧��

#endif