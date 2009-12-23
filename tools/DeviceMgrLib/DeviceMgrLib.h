#pragma once

#include <xtl.h>
#include <xboxmath.h>
#include <Xbdm.h>
#include <xbox.h>
#include <stdio.h>

typedef DWORD MAPDRIVE_STATUS;

enum
{
	DRIVE_DEVKIT	= 0,
	DRIVE_USB0		= 1,
	DRIVE_USB1		= 2,
	DRIVE_USB2		= 3,
	DRIVE_DVD		= 4,
	DRIVE_FLASH		= 5,
	DRIVE_HDD		= 6,
};

namespace DeviceMgrLib
{
	extern MAPDRIVE_STATUS dwMapState;
	enum
	{
		MAP_NONE		= 0x00000000,
		MAP_OK_USB0		= 0x00000001,
		MAP_OK_USB1		= 0x00000002,
		MAP_OK_USB2		= 0x00000004,
		MAP_OK_DVD		= 0x00000008,
		MAP_OK_FLASH	= 0x00000010,
		MAP_OK_HDD		= 0x00000020,
		MAP_OK_DEVKIT	= 0x00000040,
	};

	// ����Usb0,Usb1,Usb2,Dvd,Flash,Hdd���������غ����IsMounted_XXX�����ж�
	// �ض������Ƿ�����ȷ����
	extern MAPDRIVE_STATUS MapExternalDrives();
	// ����Xbox 360��ִ��ָ��Ŀ¼�µ�Xex.
	// ������
	// szImagePath��ִ���ļ�ȫ·������"Usb0:\\Raiden4USA\\default.xex"
	// dwFlags��    ͬ΢��⺯��XLaunchNewImage�ĵڶ�������
	//
	// ע�⣺Ŀ��xexִ�к󣬻��������ڵ�Ŀ¼Ϊ��Ϸ��Ŀ¼
	extern bool LaunchExternalImage(LPCSTR szImagePath, DWORD dwFlags);

	// �ж��ض������Ƿ�����ȷ����
	extern inline bool IsMounted_USB0()  {return (dwMapState&MAP_OK_USB0)!=0;}
	extern inline bool IsMounted_USB1()  {return (dwMapState&MAP_OK_USB1)!=0;}
	extern inline bool IsMounted_USB2()  {return (dwMapState&MAP_OK_USB2)!=0;}
	extern inline bool IsMounted_DVD()   {return (dwMapState&MAP_OK_DVD)!=0;}
	extern inline bool IsMounted_FLASH() {return (dwMapState&MAP_OK_FLASH)!=0;}
	extern inline bool IsMounted_HDD()   {return (dwMapState&MAP_OK_HDD)!=0;}
	extern inline bool IsMounted_DEVKIT(){return (dwMapState&MAP_OK_DEVKIT)!=0;}
}
