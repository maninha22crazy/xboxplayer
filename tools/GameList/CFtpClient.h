#pragma once

#ifndef _FTPCILENT_H_
#define _FTPCILENT_H_



#include <io.h>
#include <direct.h>
#include <process.h>

#include <string>

#include <xtl.h>
#include <xbdm.h>
#include <malloc.h>
#include <winsockx.h>


using namespace std;

//
//		FTP�ͻ�����
//
class CFtpClient
{
public:
	LONG m_nSIZE;												                // �������ÿ�δ��͵��ļ�����
	string m_strRecvLastInfo;									            // ��������󣬷��������ص���Ϣ

	SOCKET ConnectFtp(string strServerIp, int nPort, string strUserName, string strPassWord);
	string GetCode(char *buf);									            // ��ȡ������������
	string GetValue(char *buf);								                // ��ȡ����������ֵ
	int GetPort(string pasvStr);								            // ���ظ���URL�Ķ˿ں�
	string GetHost(string pasvStr);							        // ���ظ���URL��������ַ
	BOOL SetFilePos(SOCKET Client,LONG pos);						// ���÷������ļ�ƫ��
	SOCKET GetConnect(string host, int port);						// ����һ�����ӷ��������˿ںŵ�SOCKET
	BOOL SendCommand(SOCKET Client,string strCmd, string strInfo,LONG expCode,LONG* lpResult = NULL,string* lpOther = NULL);	// ���Ϳͻ�������
	LONG SendCommand(SOCKET Client,string strCmd, string strInfo);
	CFtpClient(void);
	~CFtpClient(void);

	static bool Startup();
};

#endif//_FTPCILENT_H_