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
//		FTP客户端类
//
class CFtpClient
{
public:
	LONG m_nSIZE;												                // 用来存放每次传送的文件长度
	string m_strRecvLastInfo;									            // 发送请求后，服务器返回的信息

	SOCKET ConnectFtp(string strServerIp, int nPort, string strUserName, string strPassWord);
	string GetCode(char *buf);									            // 获取服务器返回码
	string GetValue(char *buf);								                // 获取服务器返回值
	int GetPort(string pasvStr);								            // 返回给定URL的端口号
	string GetHost(string pasvStr);							        // 返回给定URL的主机地址
	BOOL SetFilePos(SOCKET Client,LONG pos);						// 设置服务器文件偏移
	SOCKET GetConnect(string host, int port);						// 返回一个连接服务器及端口号的SOCKET
	BOOL SendCommand(SOCKET Client,string strCmd, string strInfo,LONG expCode,LONG* lpResult = NULL,string* lpOther = NULL);	// 发送客户端请求
	LONG SendCommand(SOCKET Client,string strCmd, string strInfo);
	CFtpClient(void);
	~CFtpClient(void);

	static bool Startup();
};

#endif//_FTPCILENT_H_