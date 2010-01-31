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
#define IDS_DRIVE_HDDX							7

// �豸�ڵ���Ϣ
struct device_table 
{
    char		*deviceName;
	const wchar_t		*deviceNameW;
    UINT			deviceIndex;
    bool		isSuccess;
	bool		isUtf8;
};

// ��Ϸ�ڵ���Ϣ
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

// Arc�ڵ���Ϣ����xml�н�����
struct ArcadeInfo
{
	WCHAR strID[MAX_PATH];				// ID
	WCHAR strName[MAX_PATH];				// Name
	WCHAR strImagePath[MAX_PATH];		// ImagePath
	WCHAR strDescription[1024];		// Description
};

struct ConfigNode
{
	int   nOemCode;				// ���룺0-ʹ��ϵͳ;932-ShiftJIS(��);936(GBK����);949(��);950(Big5����)
	int   nLanguage;			// ���ػ����ԣ�0-ʹ��ϵͳ;1-Ӣ��;2-����;7-����;8-����(��);10-����(����);����ֵ-Ӣ��
	WCHAR strDevice[MAX_PATH];			// ���ѡ�е�������
	int   nShowWall;			// �Ƿ���ʾ����ͼ��Ĭ�ϴ�-1;0-�ر�
	int   nShowNewWall;			// �Ƿ���ʾ�Զ��屳��ͼ��Ĭ�Ϸ�-0;1-��
	WCHAR strWallPath[MAX_PATH];			// Ҫ��ʾ�ı���ͼ
	int   nGameType;			// ���ѡ�е���Ϸ���0-��Ϸ;1-xbla
};

typedef std::vector <GameNode> GameList;
extern struct device_table m_DeviceMappings[8];
extern LPCWSTR LocaleLanguage[11];						// ������֧��
extern ConfigNode m_ConfigNode;						// ������Ϣ
extern ArcadeInfo m_ArcadeInfo;						// ��ǰѡ�е�arc��Ϣ
extern XNADDR     m_xnaddr;

#endif