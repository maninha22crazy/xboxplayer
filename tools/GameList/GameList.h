/*=====================================================================//
//	Name:		GameList.h
//	Desc:		ͷ�ļ���ȫ�ֱ�������������
//	Coder:		GooHome��EME
//	Date:		2009-12-23
//=====================================================================*/

#ifndef GameList_H
#define GameList_H

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


/**--------------------------------------------------------------------------------------
 * LoadGameList - ��ǰĿ¼�µ�HiddenĿ¼�µĵ�һ��Ŀ¼���ض������б���
 * @m_GameList: ��Ϸ�б�����
 *
 *�ļ�Ŀ¼�ṹ��default.xex ִ���ļ�
 *				default.txt ��Ϸ�ļ�˵������������ڣ�ֱ��ʹ��Ŀ¼����
 *				default.png ��ϷͼƬ
 * Returns��û�з���ֵ��͵����
 * Author��GooHome
 * History��2009/12/12 ��������
 --------------------------------------------------------------------------------------*/
VOID LoadGameList(GameList *m_GameList);


/**--------------------------------------------------------------------------------------
 * getGameTitle - ��Ϸ�ļ�˵��
 * @lpFileName: ��Ϸ˵���ļ�
 * @lpGameName: ��Ϸ��
 *
 *�ļ�Ŀ¼�ṹ��
 *				default.txt ��Ϸ�ļ�˵������������ڣ�ֱ��ʹ��Ŀ¼����
 * Returns���ɹ���־
 * Author��GooHome
 * History��2009/12/12 ��������
 --------------------------------------------------------------------------------------*/
bool getGameTitle(char* lpFileName,char* lpGameName);


//*========================================================================//
//	Name	:	MountDevice
//  Desc	:	�����豸
//	Param	: 
//			DriveType:	Ҫ���ص��豸ID:
//										IDS_DRIVE_DEVKIT	:	DEVKITӳ���̷�
//										IDS_DRIVE_USB0		:	USB�ӿ�0
//										IDS_DRIVE_USB1		:	USB�ӿ�1
//										IDS_DRIVE_USB2		:	USB�ӿ�2
//										IDS_DRIVE_DVD		:	DVD����
//										IDS_DRIVE_FLASH		:	FLASH
//										IDS_DRIVE_HDD		:	HDDӲ��
//
//	Return	:	true - ���سɹ�;flase - ����ʧ��
//	Coder	:	EME
//	Date	:	2009-12-24
//========================================================================*/
BOOL MountDevice(UINT DriveType);

#endif