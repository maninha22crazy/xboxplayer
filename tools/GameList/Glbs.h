#include <xui.h>
#include "Utility.h"

GameList m_GameList;
INT m_nCurSel		= 0;				//	��ǰѡ����
INT m_nCurPage		= 1;				//	��ǰѡ��ҳ
INT m_nPageSize		= 0;				//	ҳ�Ŀ���ʾ��
INT m_nCountPage	= 1;				//	��ҳ��


CHAR* m_curRoot		= "";				//	�����豸�ĸ�Ŀ¼
BOOL m_IsUtf8		= false;			//	��ǰ�豸�Ƿ�utf8�����ļ���
UINT m_nCurDevice	= IDS_DRIVE_DEVKIT;	//	��ǰѡ�е��豸
CXuiControl m_lbDevice;					//	��ʾ��ǰ�豸��label

BOOL m_bSortLess	= false;			//	������

ConfigNode m_ConfigNode;				//  ������Ϣ