#include "CFtpClient.h"
#include <string>
#include "Externs.h"
#include "Utility.h"

using namespace std;

#define  BUFFER_SIZE 1024

CFtpClient::CFtpClient(void)
{
	m_nSIZE			= 20480;
}


CFtpClient::~CFtpClient(void)
{
}

bool CFtpClient::Startup()
{
	WORD wvis;
	WSADATA	wsaData;
	int err;

	// 判断socket的版本
	wvis = MAKEWORD(1,1);
	err = WSAStartup(wvis,&wsaData);

	if(err != 0)
	{
		return false;
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion ) != 1 )			// 如果socket的版本不是1.1退出
	{
		WSACleanup();
		return false; 
	}
	return true;
}

/**********************************************************
函名名称: GetConnect
描    述: 与服务器建立连接
输入参数: string host			服务器IP
int port				服务端口
输出参数: 
返 回 值: SOCKET  (SOCKET 连接成功,NULL  连接失败)
**********************************************************/
SOCKET CFtpClient::GetConnect(string host, int port)
{
	SOCKET Client=socket(AF_INET,SOCK_STREAM,0);									// 创建socket，TCP/IP协议

	// 判断地址族
	SOCKADDR_IN addrClient;
    addrClient.sin_addr.S_un.S_addr = inet_addr("192.168.73.1");
	addrClient.sin_family = AF_INET;
	addrClient.sin_port = htons(port);


    //========================== 破解 begin =================================
	BOOL bBroadcast = TRUE;
	if( setsockopt(Client, SOL_SOCKET, 0x5802, (PCSTR)&bBroadcast, sizeof(BOOL) ) != 0 )
	{
	   printf( "Failed to set socket to 5802, error\n");
	}

	if( setsockopt(Client, SOL_SOCKET, 0x5801, (PCSTR)&bBroadcast, sizeof(BOOL) ) != 0 )
	{
	   printf( "Failed to set socket to 5801, error\n");
	}
	//========================== 破解 end =================================

	if(connect(Client,(SOCKADDR*)&addrClient,sizeof(SOCKADDR))==SOCKET_ERROR)
	{
		return NULL;
	}
	return Client;
}	

/**********************************************************
函名名称: ConnectFtp
描    述: 与服务器建立连接
输入参数: 
输出参数: 
返 回 值: SOCKET  (SOCKET 连接成功,NULL  连接失败)
**********************************************************/
SOCKET CFtpClient::ConnectFtp(string strServerIp, int nPort, string strUserName, string strPassWord)
{
	SOCKET client = GetConnect(strServerIp, nPort);
	int len;
	char recvBuf[100];
	//ZeroMemory(recvBuf,sizeof(recvBuf));
	len = recv(client,recvBuf,100,0);
	if(!client)
	{
		return NULL;
	}

	if(SendCommand(client, "USER", strUserName, 331 ) )
	{
		if(SendCommand(client, "PASS", strPassWord, 230 ) )	// 服务器需要密码，发送密码
		{
			return client;																// 用户名及密码通过验证
		}
	}
	return NULL;
}

/**********************************************************
函名名称: SetFilePos
描    述: 向服务器上文件发送偏移请求
输入参数: SOCKET Client			连接服务器的SOCKET
LONG pos				偏移量
输出参数: 
返 回 值: BOOL
**********************************************************/
BOOL CFtpClient::SetFilePos(SOCKET Client,LONG pos)
{
	string strPos;
	char buffer[20]; 
	ltoa(pos,buffer,10);
	strPos = buffer;

	if(!SendCommand(Client,"REST",strPos,350))
	{
		return false;				// 定位出错	
	}
	return true;
}

/**********************************************************
函名名称: SendCommand
描    述: 向服务器发送命令
输入参数: SOCKET Client			连接服务器的SOCKET
string strCmd			要发送的命令
string code			返回的代码
输出参数: 
返 回 值: BOOL
**********************************************************/
BOOL CFtpClient::SendCommand(SOCKET Client,string strCmd, string strInfo,LONG expCode,LONG* lpResult,string* lpOther)
{
	LONG len = 0,lResponse = 0;
	char sendBuf[BUFFER_SIZE+1],recvBuf[BUFFER_SIZE+1];

	ZeroMemory(sendBuf,sizeof(sendBuf));
	ZeroMemory(recvBuf,sizeof(recvBuf));

	// 构建命令
	//USES_CONVERSION; 
    _snprintf_s(sendBuf, BUFFER_SIZE, "%s %s\r\n", strCmd.c_str(), strInfo.c_str());

	send(Client,sendBuf,strlen(sendBuf),0);
	len = recv(Client,recvBuf,BUFFER_SIZE,0);
	if (len < 0)
	{
		Sleep(200);
		len = recv(Client,recvBuf,BUFFER_SIZE,0);
		if (len < 0)
		{
			return false;
		}
	}
	char* szStop;
	lResponse = strtol(recvBuf,&szStop,10);

	if (lpResult)
	{
		*lpResult = lResponse;
	}
	if (lpOther)
	{
		*lpOther = string(szStop);
	}

	m_strRecvLastInfo = recvBuf;

	return expCode == lResponse;
}

/**********************************************************
函名名称: SendCommand
描    述: 向服务器发送命令
输入参数: SOCKET Client			连接服务器的SOCKET
string strCmd			要发送的命令
string code			返回的代码
输出参数: 
返 回 值: LONG
**********************************************************/
LONG CFtpClient::SendCommand(SOCKET Client,string strCmd, string strInfo)
{
	LONG len = 0,lResponse = 0;
	char sendBuf[BUFFER_SIZE+1],recvBuf[BUFFER_SIZE+1];

	ZeroMemory(sendBuf,sizeof(sendBuf));
	ZeroMemory(recvBuf,sizeof(recvBuf));

	// 构建命令
	//USES_CONVERSION; 
    _snprintf_s(sendBuf,BUFFER_SIZE,"%s %s\r\n",strCmd.c_str(), strInfo.c_str());

	send(Client,sendBuf,strlen(sendBuf),0);
	len = recv(Client,recvBuf,BUFFER_SIZE,0);
	if (len < 0)
	{
		Sleep(200);
		len = recv(Client,recvBuf,BUFFER_SIZE,0);
		if (len < 0)
		{
			return 0;
		}
	}
	char* szStop;
	lResponse = strtol(recvBuf,&szStop,10);
	m_strRecvLastInfo = recvBuf;

	return lResponse;
}

/**********************************************************
函名名称: GetCode
描    述: 获取服务器返回码
输入参数: char *buf			给定的字符串
输出参数: 
返 回 值: string
**********************************************************/
string CFtpClient::GetCode(char *buf)
{
	string str;
	str = buf;
	try
	{
        str=str.substr(0,3);
		return str;
	}
	catch (...)
	{
		return "";
	}
}

/**********************************************************
函名名称: GetValue
描    述: 获取服务器返回值
输入参数: char *buf			给定的字符串
输出参数: 
返 回 值: string
**********************************************************/
string CFtpClient::GetValue(char *buf)
{
	string str;
	str = buf;
	try
	{
        if(str.length() > 4)
		{
			str = str.substr(4,  str.length() - 4);
		}
		return str;
	}
	catch (...)
	{
		return "";
	}
}


/**********************************************************
函名名称: GetHost
描    述: 从给定字符中分离出IP地址
输入参数: string pasvStr			给定的字符串
输出参数: 
返 回 值: string
**********************************************************/
string CFtpClient::GetHost(string pasvStr)
{
    int index = pasvStr.rfind('(');
	//取出"()"中的部份
	string sIP;
	string temp;
    temp = pasvStr.substr(index+2, pasvStr.length()-index-2);

	//取出最后一个","后面的数字
	int i=temp.rfind(',');
    string sPort2=temp.substr(i + 1, temp.length()-i-1);
    temp=temp.substr(0, i);

	//取出倒数第二个","后面的数字
	i = temp.rfind(',');
	string sPort1=temp.substr(i + 1,temp.length()-i-1);
	temp=temp.substr(0, i);
	//将","改为"."组成服器的IP地址
    temp = replace_all(temp,",", ".");
	//返回分离出来的IP地址
	return temp;
}


/**********************************************************
函名名称: GetPort
描    述: 从给定字符中分离出端口号
输入参数: string pasvStr			给定的字符串
输出参数: 
返 回 值: int
**********************************************************/
int CFtpClient::GetPort(string pasvStr)
{
	int index = pasvStr.rfind('(');
	string temp;												// 取出"()"中的部份
	temp = pasvStr.substr(index+1, pasvStr.length()-index-2);
	int i = temp.rfind(',');
	string sPort2 = temp.substr(i + 1,temp.length()-i-1);			// 取出最后一个","后面的数字
    temp = temp.substr(0, i);
	i = temp.rfind(',');
    string sPort1=temp.substr(i + 1, temp.length()-i-1);			// 取出倒数第二个","后面的数字

    int port1 = atoi(sPort1.c_str());									// 将分别分离出来的字符转换成数字
    int port2 = atoi(sPort2.c_str());

	//计算得出临时端口号并返回
	return port1*256 + port2;
}