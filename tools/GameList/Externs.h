#include "Utility.h"

extern GameList m_GameList;
extern INT m_nCurSel;				//	��ǰѡ����
extern INT m_nCurPage;				//	��ǰѡ��ҳ
extern INT m_nPageSize;			//	ҳ�Ŀ���ʾ��
extern INT m_nCountPage;			//	��ҳ��


//extern CHAR* m_curRoot;			//	�����豸�ĸ�Ŀ¼
//extern BOOL m_IsUtf8;				//	��ǰ�豸�Ƿ�utf8�����ļ���

extern BOOL m_bSortLess;			//	������

extern ConfigNode m_ConfigNode;	//  ������Ϣ

extern CHAR* m_strConfigPath;				// �����ļ�λ��
extern WCHAR* m_strBackGroundPath;			// ����ͼ�ļ�λ��
extern WCHAR* m_strGameList;					//	��Ϸ�б�����

extern LPCWSTR LocaleLanguage[11];			// ������֧��