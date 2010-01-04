#include <xui.h>
#include "Utility.h"

GameList m_GameList;
INT m_nCurSel		= 0;				//	当前选中项
INT m_nCurPage		= 1;				//	当前选中页
INT m_nPageSize		= 0;				//	页的可显示项
INT m_nCountPage	= 1;				//	总页数


CHAR* m_curRoot		= "";				//	查找设备的根目录
BOOL m_IsUtf8		= false;			//	当前设备是否utf8命名文件名
UINT m_nCurDevice	= IDS_DRIVE_DEVKIT;	//	当前选中的设备
CXuiControl m_lbDevice;					//	显示当前设备的label

BOOL m_bSortLess	= false;			//	升降序

ConfigNode m_ConfigNode;				//  配置信息