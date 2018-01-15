#ifndef SCLIENT_H_INCLUDED
#define SCLIENT_H_INCLUDED
#include <winsock2.h>

#include <iostream>
using namespace std;


#define TIMEFOR_THREAD_CLIENT		500		//thread sleep time

#define	MAX_NUM_CLIENT		10				//all connection numbers from client
#define	MAX_NUM_BUF			1024*8				//max length for buffer
#define INVALID_OPERATOR	1				//invalid operation
#define INVALID_NUM			2				//denominator is ZERO
#define ZERO				0				//ZERO


//data packet head struct, it's 4bytes in win32
typedef struct _head
{
	char			type;	//type
	unsigned short	len;	//length of data packet(include packet head)
}hdr, *phdr;

//data struct in data packet
typedef struct _data
{
	char	buf[MAX_NUM_BUF];//data
}DATABUF, *pDataBuf;


class CClient
{
public:
	CClient(const SOCKET sClient,const sockaddr_in &addrClient);
	virtual ~CClient();

public:
	BOOL		StartRuning(void);					//create thread to send and recv data
	void		HandleData(const char* pExpr);		//caculation expression
	BOOL		IsConning(void){					//if connection exists
				return m_bConning;
				}
	void		DisConning(void){					//disconnect client
				m_bConning = FALSE;
				}
	BOOL		IsExit(void){						//if recv and send threads are exit
				return m_bExit;
				}
    BOOL		IsSend(void){
				m_bSend = TRUE;
				return m_bSend;
				}
   void		SetReadyToSend(BOOL flag){
			m_bReadyToSend = flag;
			}
   
   void		SetSendConnectionSuccess(BOOL flag){
			m_bSendConnectionSuccess = flag;
			}
   void		SetReadyToSendStr(BOOL flag){
	//		m_bReadyToSendStr = flag;
			}
   BOOL 	IsReadyToSendStr(void){
//			return m_bReadyToSendStr;
   			}
public:
	static DWORD __stdcall RecvDataThread(void* pParam);		//recv data from client
	static DWORD __stdcall SendDataThread(void* pParam);		//send data to client

private:
	CClient();
private:
	SOCKET		m_socket;			//socket
	sockaddr_in	m_addr;				//address
	DATABUF		m_data;				//data
	HANDLE		m_hEvent;			//event
	HANDLE		m_hThreadSend;		//thread handler for sending data
	HANDLE		m_hThreadRecv;		//thread handler for receiving data
	CRITICAL_SECTION m_cs;			//critical section
	BOOL		m_bConning;			//client connection status
	BOOL        m_bSend;            //data sending status
	BOOL		m_bExit;			//thread exit
	BOOL		m_bReadyToSend;
	BOOL 		m_bSendConnectionSuccess;
//	BOOL		m_bReadyToSendStr;
};


#endif // SCLIENT_H_INCLUDED
