#include "Utility.h"

extern GameList m_GameList;
extern INT m_nCurSel;				//	当前选中项
extern INT m_nCurPage;				//	当前选中页
extern INT m_nPageSize;			//	页的可显示项
extern INT m_nCountPage;			//	总页数


//extern CHAR* m_curRoot;			//	查找设备的根目录
extern BOOL m_IsUtf8;				//	当前设备是否utf8命名文件名
extern UINT m_nCurDevice;			//	当前选中的设备

extern BOOL m_bSortLess;			//	升降序

extern ConfigNode m_ConfigNode;	//  配置信息