/*=====================================================================//
//	Name:		GameList.h
//	Desc:		头文件、全局变量、方法定义
//	Coder:		GooHome、EME
//	Date:		2009-12-23
//=====================================================================*/

#ifndef GameList_H
#define GameList_H


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

//*========================================================================//
//	Name	:	SortList
//  Desc	:	对游戏列表进行排序
//	Param	: 
//			SortType:	排序的类型:
//										0	:	创建时间
//										1	:	其他...
//
//	Return	:	true - 挂载成功;flase - 挂载失败
//	Coder	:	EME
//	Date	:	2009-12-24
//========================================================================*/
VOID SortGameList(GameList *m_GameList, UINT SortType);

//*========================================================================//
//	Name	:	LoadConfig
//  Desc	:	读取配置文件
//	Param	: 
//	Return	:	没有返回值
//	Coder	:	EME
//	Date	:	2009-12-30
//========================================================================*/
VOID LoadConfig(VOID);
#endif