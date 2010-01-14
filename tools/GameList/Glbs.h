#include <xui.h>
#include "Utility.h"

#define IDS_JP			932				// ����Shift-JIS
#define IDS_CHS		936				// ���ļ���
#define IDS_KOREAN		949				// ����
#define IDS_CHT		950				// ���ķ���Big5

GameList m_GameList;
INT m_nCurSel		= 0;				//	��ǰѡ����
INT m_nCurPage		= 1;				//	��ǰѡ��ҳ
INT m_nPageSize		= 0;				//	ҳ�Ŀ���ʾ��
INT m_nCountPage	= 1;				//	��ҳ��


//CHAR* m_curRoot		= "";				//	�����豸�ĸ�Ŀ¼
BOOL m_IsUtf8		= false;			//	��ǰ�豸�Ƿ�utf8�����ļ���
//UINT m_nCurDevice	= IDS_DRIVE_DEVKIT;	//	��ǰѡ�е��豸
CXuiControl m_lbDevice;					//	��ʾ��ǰ�豸��label

BOOL m_bSortLess	= false;			//	������

CHAR* m_curRoot		= "";				//	�����豸�ĸ�Ŀ¼
ConfigNode m_ConfigNode;				//  ������Ϣ

CHAR* m_strConfigPath	= "game:\\GameList.xml";	// �����ļ�λ��

// ������֧��
LPCWSTR LocaleLanguage[11] =
{
	L"en-en",	// the default locale
	L"en-en",	// English
	L"ja-jp",	// Japanese
	L"de-de",	// German
	L"fr-fr",	// French
	L"es-es",	// Spanish
	L"it-it",	// Italian
	L"ko-kr",	// Korean
	L"zh-cht",	// Traditional Chinese
	L"pt-pt",	// Portuguese
	L"zh-chs"	// Simplified Chinese
};