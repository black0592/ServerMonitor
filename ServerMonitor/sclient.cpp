#include "stdafx.h"
#include <process.h>
#include <stdio.h>
#include "sclient.h"
#include "server.h"
#include <thread>

#define SIZE 1024*8

//extern BOOL bSend;
//extern char	dataBuf[MAX_NUM_BUF];
/*
 * construct function
 */
#define LOGFILESERVER "F:\\LLVRMonitor.txt"
SYSTEMTIME sys;
char sendMsgLog[1024];
char sendMsgLogA[1024];
char sendTime[256];

int WriteToLog(char* str)
{
	FILE* log;
	GetLocalTime(&sys);
	sprintf_s(sendTime, "%02d/%02d %02d:%02d:%02d.%03d ", sys.wMonth, sys.wDay, sys.wHour,
		sys.wMinute, sys.wSecond, sys.wMilliseconds);
	memset(sendMsgLog, 0, 1024);
	sprintf_s(sendMsgLog, "%s%s", sendTime, str);
	fopen_s(&log, "F:\\LLVRMonitor.txt", "a+");
	if (log == NULL)
		return -1;
	fprintf(log, "%s\n ", sendMsgLog);
	fclose(log);
	return 0;
}


CClient::CClient(const SOCKET sClient, const sockaddr_in &addrClient)
{
	//init variables
	m_hThreadRecv = NULL;
	m_hThreadSend = NULL;
	m_socket = sClient;
	m_addr = addrClient;
	m_bConning = FALSE;
	m_bExit = FALSE;
	m_bSend = FALSE;
	m_bReadyToSend = FALSE;
	m_bSendConnectionSuccess = FALSE;
	memset(m_data.buf, 0, MAX_NUM_BUF);

	//create event
	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);//create event status manually, init as no signal status

	//init critical sections
	InitializeCriticalSection(&m_cs);
}
/*
 * descontruct function
 */
CClient::~CClient()
{
	closesocket(m_socket);			//close socket
	m_socket = INVALID_SOCKET;		//socket invalid
	DeleteCriticalSection(&m_cs);	//release critical section
	CloseHandle(m_hEvent);			//release event section
}

/*
 * create thread to send and receive data
 */
BOOL CClient::StartRuning(void)
{
	WriteToLog("StartRuning");
	m_bConning = TRUE;//set as connection status

#if 0	
	auto funcRecv = std::bind(&CClient::RecvDataThread, this);
	std::thread recvHandler(&CClient::RecvDataThread, this);
	recvHandler.detach(); // detach thread &bg
	//if (recvHandler.joinable())// thread should not joinable
	//{
	//	return FALSE;
	//}

//	auto funcSend = std::bind(&CClient::SendDataThread, this);
	//std::thread sendHandler(&CClient::SendDataThread, this);
	//sendHandler.detach(); // detach thread &bg
	//if (sendHandler.joinable())// thread should not joinable
	//{
	//	return FALSE;
	//}
#else
	//create thread to receive data
	unsigned long ulThreadId;
	m_hThreadRecv = CreateThread(NULL, 0, RecvDataThread, this, 0, &ulThreadId);
	if(NULL == m_hThreadRecv)
	{
		return FALSE;
	}else{
		CloseHandle(m_hThreadRecv);
	}

	//create thread to receive client data
	m_hThreadSend =  CreateThread(NULL, 0, SendDataThread, this, 0, &ulThreadId);
	if(NULL == m_hThreadSend)
	{
		return FALSE;
	}else{
		CloseHandle(m_hThreadSend);
	}
#endif
	return TRUE;
}


/*
 * receive data from client
 */
DWORD   CClient::RecvDataThread(void* pParam)
{
	CClient *pClient = (CClient*)pParam;	//pointer to client
	int		reVal;							//return value
	char	temp[MAX_NUM_BUF];				//temp value

	WriteToLog("RecvDataThread");
	cout <<"RecvDataThread"<<endl;

	while(pClient->m_bConning)				//connection status
	{	
	//	if(!pClient->m_bSendConnectionSuccess)
	//	{
	//		break;
	//	}
		cout <<"pClient->m_bConning"<<endl;
	    memset(temp, 0, MAX_NUM_BUF);
		reVal = recv(pClient->m_socket, temp, MAX_NUM_BUF, 0);	//receive data
		
		//handle error return values
		if (SOCKET_ERROR == reVal)
		{
			int nErrCode = WSAGetLastError();

			if ( WSAEWOULDBLOCK == nErrCode )	//receive data buffer is invalid
			{
				continue;						//continue loop
			}else if (WSAENETDOWN == nErrCode ||//client close connection
					 WSAETIMEDOUT == nErrCode ||
					WSAECONNRESET == nErrCode )
			{
				break;							//thread exit
			}
		}

		//client close the connection
		if ( reVal == 0)
		{
			break;
		}

		//receive data
		if (reVal > 0)
		{
			cout <<"reVal > 0"<<endl;
		    EnterCriticalSection(&pClient->m_cs);
		    char *pClientIP = inet_ntoa(pClient->m_addr.sin_addr);
            u_short  clientPort = ntohs(pClient->m_addr.sin_port);
		//	cout<<"IP: "<<pClientIP<<"\tPort: "<<clientPort<<":"<<temp<<endl;      //output and show data
			WriteToLog(temp);
			char file_name[MAX_NUM_BUF];
			char *pfile = temp; // indicate path
			memset(file_name, 0, MAX_NUM_BUF);
			strncpy_s(file_name, "F:\\receive\\", strlen("F:\\receive\\"));
			int file_len = strlen(temp), i = 0, tem = 0;

			for (i = 0; i < file_len; i++)
			{
				if (strncmp(pfile + file_len - 1 -i, "\\", 1))  //if equal, return 0; else, return Positive
				{
					tem++;
					continue; //not equal
				}
				else // if equal, strcat path after \\  to file_name, it's exact length of path after 
					{
					 	strncat_s(file_name, pfile + file_len - i, i);
						break;
					}
			}
			
			if (tem == file_len)
			{
				strncat_s(file_name, temp, strlen(temp) > 1024 ? 1024 : strlen(temp));
			}

			FILE *fp;
			fopen_s(&fp, file_name, "wb");
	    	int len = send(pClient->m_socket, "Ready to send", strlen("Ready to send") + 1, 0);
			memset(sendMsgLogA, 0, 1024);
			sprintf_s(sendMsgLogA, "%s%d", "send size: ", len );
			WriteToLog(sendMsgLogA);
			
			char datalength[20];
			long int length = 0;
			memset(datalength, 0, 20);
			int lenRecv = recv(pClient->m_socket, datalength, 21, 0);  //send file size from Client
			length = atol(datalength);

			memset(sendMsgLogA, 0, 1024);
			sprintf_s(sendMsgLogA, "%s%ld%s%ld", "file size: ", length, " recv size:", lenRecv);
			WriteToLog(sendMsgLogA);				//ready to send

			double cent = 0.0;
			char receiveBuf[SIZE] = {'0'};
		//	memset(receiveBuf, 0, SIZE);
			long int x = 0;
			while (1)
			{	
				memset(receiveBuf, 0, SIZE);
				cout <<"while (1)"<<endl;
				x = x + SIZE;  //SIZE scope is from -128 to 127
				if (x < length) // ZJX
				{
					cent = (double)x*100.0 / (double)length;
					memset(sendMsgLogA, 0, 1024);
					sprintf_s(sendMsgLogA, "%s%4.2f", "have received: ", cent);
					WriteToLog(sendMsgLogA);				//ready to send
					recv(pClient->m_socket, receiveBuf, SIZE + 1, 0);      //recv SIZE files
					fwrite(receiveBuf, 1, SIZE, fp);             //write SIZE files into receiveBuf, and continue to loop
				}
				else  //excute the function directory while files is smaller, and loop exit
				{
					recv(pClient->m_socket, receiveBuf, length + SIZE - x + 1, 0);
					fwrite(receiveBuf, 1, length + SIZE - x, fp);
					fclose(fp);						
					WriteToLog("file received done");
					break;
				}

			}
		}
		WriteToLog("out of reVal>0");
			
		LeaveCriticalSection(&pClient->m_cs);
		WriteToLog("out of LeaveCriticalSection");
		memset(temp, 0, MAX_NUM_BUF);	//clean up temp variables
	}
		
	WriteToLog("out of pClient->m_bConning");
	pClient->m_bConning = FALSE;			//disconnect with client
	return 0;								//thread exit
}

/*
 * @des: send data to client
 */
DWORD CClient::SendDataThread(void* pParam)
{
	WriteToLog("SendDataThread");
	cout <<"SendDataThread"<<endl;
	CClient *pClient = (CClient*)pParam;//convert data type to pointer CClient
	while(pClient->m_bConning)//connection status
	{
		cout <<"SendDataThread m_bConning"<<endl;
        if(pClient->m_bSend || bSend)
        {
			//enter data critical sections
			EnterCriticalSection(&pClient->m_cs);
			//send data
			int val = send(pClient->m_socket, dataBuf, strlen(dataBuf),0);	
			//int val = send(pClient->m_socket, "Connect to Server successfully", strlen("Connect to Server successfully"),0);
			//handle data of return values
			if (SOCKET_ERROR == val)
			{
				int nErrCode = WSAGetLastError();
				if (nErrCode == WSAEWOULDBLOCK)//send buffer invalid
				{
					continue;
				}else if ( WSAENETDOWN == nErrCode ||
						  WSAETIMEDOUT == nErrCode ||
						  WSAECONNRESET == nErrCode)//client disconnection
				{
					//leave critical section
					LeaveCriticalSection(&pClient->m_cs);

					pClient->m_bConning = FALSE;	//disconnect connection
//					pClient->m_bExit = TRUE;		//thread exit
					pClient->m_bSend = FALSE;
					break;
				}else {
					//leave critical section
					LeaveCriticalSection(&pClient->m_cs);
					pClient->m_bConning = FALSE;	//connection disconnect
//					pClient->m_bExit = TRUE;		//thread exit
					pClient->m_bSend = FALSE;
					break;
				}
			}
			//send data successfully
			//leave critical section
			memset(dataBuf, 0, MAX_NUM_BUF);		//cleanup receive buffer
			LeaveCriticalSection(&pClient->m_cs);
			//set event as no signal status
			pClient->m_bSend = FALSE;
			bSend = FALSE;
		}

	}

	return 0;
}

