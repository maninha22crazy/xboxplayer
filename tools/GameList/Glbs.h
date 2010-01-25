#include <xui.h>
#include "Externs.h"



struct device_table m_DeviceMappings[] = {
			{ "Hdd",L"Hdd",  IDS_DRIVE_HDD,  false,  false  },
			{ "Devkit",L"Devkit",  IDS_DRIVE_DEVKIT,  false,  false  },
	        { "Usb0",L"Usb0",   IDS_DRIVE_USB0,  false,	  true  },
			{ "Usb1",L"Usb1",   IDS_DRIVE_USB1,  false,	  true  },
			{ "Usb2",L"Usb2",   IDS_DRIVE_USB2,  false,	  true  },
			{ "Dvd", L"Dvd",  IDS_DRIVE_DVD,  false,	  true  },
			{ "Dvd", L"Flash",  IDS_DRIVE_FLASH,  false,	  true  }
			};


CXuiTextElement m_lbGameTitle;	



struct device_table m_curDevice;
ConfigNode m_ConfigNode;				//  ≈‰÷√–≈œ¢