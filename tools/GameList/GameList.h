/*=====================================================================//
//	Name:		GameList.h
//	Desc:		头文件、全局变量、方法定义
//	Coder:		GooHome、EME
//	Date:		2009-12-23
//=====================================================================*/

#ifndef GameList_H
#define GameList_H

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


/**--------------------------------------------------------------------------------------
 * LoadGameList - 当前目录下的Hidden目录下的第一层目录加载都向量列表里
 * @m_GameList: 游戏列表数组
 *
 *文件目录结构：default.xex 执行文件
 *				default.txt 游戏文件说明（如果不存在，直接使用目录名）
 *				default.png 游戏图片
 * Returns：没有返回值（偷懒）
 * Author：GooHome
 * History：2009/12/12 初版做成
 --------------------------------------------------------------------------------------*/
VOID LoadGameList(GameList *m_GameList);


/**--------------------------------------------------------------------------------------
 * getGameTitle - 游戏文件说明
 * @lpFileName: 游戏说明文件
 * @lpGameName: 游戏名
 *
 *文件目录结构：
 *				default.txt 游戏文件说明（如果不存在，直接使用目录名）
 * Returns：成功标志
 * Author：GooHome
 * History：2009/12/12 初版做成
 --------------------------------------------------------------------------------------*/
bool getGameTitle(char* lpFileName,char* lpGameName);


//*========================================================================//
//	Name	:	MountDevice
//  Desc	:	挂载设备
//	Param	: 
//			DriveType:	要挂载的设备ID:
//										IDS_DRIVE_DEVKIT	:	DEVKIT映射盘符
//										IDS_DRIVE_USB0		:	USB接口0
//										IDS_DRIVE_USB1		:	USB接口1
//										IDS_DRIVE_USB2		:	USB接口2
//										IDS_DRIVE_DVD		:	DVD光驱
//										IDS_DRIVE_FLASH		:	FLASH
//										IDS_DRIVE_HDD		:	HDD硬盘
//
//	Return	:	true - 挂载成功;flase - 挂载失败
//	Coder	:	EME
//	Date	:	2009-12-24
//========================================================================*/
BOOL MountDevice(UINT DriveType);

#endif