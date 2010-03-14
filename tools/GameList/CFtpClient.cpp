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

	// �ж�socket�İ汾
	wvis = MAKEWORD(1,1);
	err = WSAStartup(wvis,&wsaData);

	if(err != 0)
	{
		return false;
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion ) != 1 )			// ���socket�İ汾����1.1�˳�
	{
		WSACleanup();
		return false; 
	}
	return true;
}

/**********************************************************
��������: GetConnect
��    ��: ���������������
�������: string host			������IP
int port				����˿�
�������: 
�� �� ֵ: SOCKET  (SOCKET ���ӳɹ�,NULL  ����ʧ��)
**********************************************************/
SOCKET CFtpClient::GetConnect(string host, int port)
{
	SOCKET Client=socket(AF_INET,SOCK_STREAM,0);									// ����socket��TCP/IPЭ��

	// �жϵ�ַ��
	SOCKADDR_IN addrClient;
    addrClient.sin_addr.S_un.S_addr = inet_addr("192.168.73.1");
	addrClient.sin_family = AF_INET;
	addrClient.sin_port = htons(port);


    //========================== �ƽ� begin =================================
	BOOL bBroadcast = TRUE;
	if( setsockopt(Client, SOL_SOCKET, 0x5802, (PCSTR)&bBroadcast, sizeof(BOOL) ) != 0 )
	{
	   printf( "Failed to set socket to 5802, error\n");
	}

	if( setsockopt(Client, SOL_SOCKET, 0x5801, (PCSTR)&bBroadcast, sizeof(BOOL) ) != 0 )
	{
	   printf( "Failed to set socket to 5801, error\n");
	}
	//========================== �ƽ� end =================================

	if(connect(Client,(SOCKADDR*)&addrClient,sizeof(SOCKADDR))==SOCKET_ERROR)
	{
		return NULL;
	}
	return Client;
}	

/**********************************************************
��������: ConnectFtp
��    ��: ���������������
�������: 
�������: 
�� �� ֵ: SOCKET  (SOCKET ���ӳɹ�,NULL  ����ʧ��)
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
		if(SendCommand(client, "PASS", strPassWord, 230 ) )	// ��������Ҫ���룬��������
		{
			return client;																// �û���������ͨ����֤
		}
	}
	return NULL;
}

/**********************************************************
��������: SetFilePos
��    ��: ����������ļ�����ƫ������
�������: SOCKET Client			���ӷ�������SOCKET
LONG pos				ƫ����
�������: 
�� �� ֵ: BOOL
**********************************************************/
BOOL CFtpClient::SetFilePos(SOCKET Client,LONG pos)
{
	string strPos;
	char buffer[20]; 
	ltoa(pos,buffer,10);
	strPos = buffer;

	if(!SendCommand(Client,"REST",strPos,350))
	{
		return false;				// ��λ����	
	}
	return true;
}

/**********************************************************
��������: SendCommand
��    ��: ���������������
�������: SOCKET Client			���ӷ�������SOCKET
string strCmd			Ҫ���͵�����
string code			���صĴ���
�������: 
�� �� ֵ: BOOL
**********************************************************/
BOOL CFtpClient::SendCommand(SOCKET Client,string strCmd, string strInfo,LONG expCode,LONG* lpResult,string* lpOther)
{
	LONG len = 0,lResponse = 0;
	char sendBuf[BUFFER_SIZE+1],recvBuf[BUFFER_SIZE+1];

	ZeroMemory(sendBuf,sizeof(sendBuf));
	ZeroMemory(recvBuf,sizeof(recvBuf));

	// ��������
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
��������: SendCommand
��    ��: ���������������
�������: SOCKET Client			���ӷ�������SOCKET
string strCmd			Ҫ���͵�����
string code			���صĴ���
�������: 
�� �� ֵ: LONG
**********************************************************/
LONG CFtpClient::SendCommand(SOCKET Client,string strCmd, string strInfo)
{
	LONG len = 0,lResponse = 0;
	char sendBuf[BUFFER_SIZE+1],recvBuf[BUFFER_SIZE+1];

	ZeroMemory(sendBuf,sizeof(sendBuf));
	ZeroMemory(recvBuf,sizeof(recvBuf));

	// ��������
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
��������: GetCode
��    ��: ��ȡ������������
�������: char *buf			�������ַ���
�������: 
�� �� ֵ: string
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
��������: GetValue
��    ��: ��ȡ����������ֵ
�������: char *buf			�������ַ���
�������: 
�� �� ֵ: string
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
��������: GetHost
��    ��: �Ӹ����ַ��з����IP��ַ
�������: string pasvStr			�������ַ���
�������: 
�� �� ֵ: string
**********************************************************/
string CFtpClient::GetHost(string pasvStr)
{
    int index = pasvStr.rfind('(');
	//ȡ��"()"�еĲ���
	string sIP;
	string temp;
    temp = pasvStr.substr(index+2, pasvStr.length()-index-2);

	//ȡ�����һ��","���������
	int i=temp.rfind(',');
    string sPort2=temp.substr(i + 1, temp.length()-i-1);
    temp=temp.substr(0, i);

	//ȡ�������ڶ���","���������
	i = temp.rfind(',');
	string sPort1=temp.substr(i + 1,temp.length()-i-1);
	temp=temp.substr(0, i);
	//��","��Ϊ"."��ɷ�����IP��ַ
    temp = replace_all(temp,",", ".");
	//���ط��������IP��ַ
	return temp;
}


/**********************************************************
��������: GetPort
��    ��: �Ӹ����ַ��з�����˿ں�
�������: string pasvStr			�������ַ���
�������: 
�� �� ֵ: int
**********************************************************/
int CFtpClient::GetPort(string pasvStr)
{
	int index = pasvStr.rfind('(');
	string temp;												// ȡ��"()"�еĲ���
	temp = pasvStr.substr(index+1, pasvStr.length()-index-2);
	int i = temp.rfind(',');
	string sPort2 = temp.substr(i + 1,temp.length()-i-1);			// ȡ�����һ��","���������
    temp = temp.substr(0, i);
	i = temp.rfind(',');
    string sPort1=temp.substr(i + 1, temp.length()-i-1);			// ȡ�������ڶ���","���������

    int port1 = atoi(sPort1.c_str());									// ���ֱ����������ַ�ת��������
    int port2 = atoi(sPort2.c_str());

	//����ó���ʱ�˿ںŲ�����
	return port1*256 + port2;
}