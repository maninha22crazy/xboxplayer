#include <xui.h>
#include "Externs.h"

struct device_table m_DeviceMappings[] = {
			{ "Devkit",L"Devkit",  IDS_DRIVE_HDD,  false,  false  },
			{ "Hdd",L"Hdd",  IDS_DRIVE_HDD,  false,  false  },
	        { "Usb0",L"Usb0",   IDS_DRIVE_USB0,  false,	  true  },
			{ "Usb1",L"Usb1",   IDS_DRIVE_USB1,  false,	  true  },
			{ "Usb2",L"Usb2",   IDS_DRIVE_USB2,  false,	  true  },
			{ "Dvd", L"Dvd",  IDS_DRIVE_DVD,  false,	  false  },
			{ "Flash", L"Flash",  IDS_DRIVE_FLASH,  false,	  false  },
			{ "HddX", L"HddX",  IDS_DRIVE_HDDX,  false,	  false  }
			};

struct device_table m_curDevice;
ConfigNode m_ConfigNode;				//  配置信息
ArcadeInfo m_ArcadeInfo;				//  当前选中的arc信息
XNADDR     m_xnaddr;