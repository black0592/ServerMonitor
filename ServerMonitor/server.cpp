#include "stdafx.h"
#include "server.h"
#include "sclient.h"


/**
 * gloable variables
 */
char	dataBuf[MAX_NUM_BUF];				//written buffer
BOOL	bConning;							//connection status with Client
BOOL    bSend;                              //send flag
BOOL    clientConn;                         //connect client flag
SOCKET	sServer;							//server listen socket
CRITICAL_SECTION  cs;			            //critical section for protecting data
HANDLE	hAcceptThread;						//data processing handler
HANDLE	hCleanThread;						//data receiving thread
ClIENTVECTOR clientvector;                  //store child sockets
BOOL m_bReadyToSendStr;

/**
 * Init
 */
BOOL initSever(void)
{
    //init gloable variables
	initMember();

	//init SOCKET
	if (!initSocket())
		return FALSE;

	return TRUE;
}

/**
 * init gloable variables
 */
void initMember(void)
{
	InitializeCriticalSection(&cs);				            //init critical section
	memset(dataBuf, 0, MAX_NUM_BUF);
	bSend = FALSE;
	clientConn = FALSE;
	bConning = FALSE;									    //server with no running status
	hAcceptThread = NULL;									//set as NULL
	hCleanThread = NULL;
	sServer = INVALID_SOCKET;								//set as invalid socket
	clientvector.clear();									//cleanup vector
}

/**
 *  init SOCKET
 */
bool initSocket(void)
{
	//return value
	int reVal;

	//init Windows Sockets DLL
	WSADATA  wsData;
	reVal = WSAStartup(MAKEWORD(2,2),&wsData);

	//create socket
	sServer = socket(AF_INET, SOCK_STREAM, 0);
	if(INVALID_SOCKET== sServer)
		return FALSE;

	//set socket as non-block mode
	unsigned long ul = 1;
	reVal = ioctlsocket(sServer, FIONBIO, (unsigned long*)&ul);
	if (SOCKET_ERROR == reVal)
		return FALSE;

	//bind socket
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SERVERPORT);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	reVal = bind(sServer, (struct sockaddr*)&serAddr, sizeof(serAddr));
	if(SOCKET_ERROR == reVal )
		return FALSE;

	//listen
	reVal = listen(sServer, CONN_NUM);
	if(SOCKET_ERROR == reVal)
		return FALSE;

	return TRUE;
}

/**
 *  start server
 */
bool startService(void)
{
    BOOL reVal = TRUE;	//return values

	showTipMsg(START_SERVER);	//remind users to input

	char cInput;		//input characters
	do
	{
		//cin >> cInput;
		cInput = 's';
		if ('s' == cInput || 'S' == cInput)
		{
			if (createCleanAndAcceptThread())	//thread of receiving client request
			{
				showServerStartMsg(TRUE);		//show msg of creating thread successfully
			}
			else
				{
					reVal = FALSE;
				}
			break;//skip loop
		}
		else
			{
			//	showTipMsg(START_SERVER);
			}

	} while(cInput != 's' && cInput != 'S'); //must input character 's'or 'S'

    cin.sync();                     //cleanup input buffer
	return reVal;

}

/**
 * create thread to cleanup resource and accept client request
 */
BOOL createCleanAndAcceptThread(void)
{
    bConning = TRUE;//set server as running status

	//create release resource thread
	unsigned long ulThreadId;
	//create thread to accept client request
	hAcceptThread = CreateThread(NULL, 0, acceptThread, NULL, 0, &ulThreadId);
	if( NULL == hAcceptThread)
	{
		bConning = FALSE;
		return FALSE;
	}
	else
    {
		CloseHandle(hAcceptThread);
	}
	//create thread to accept data
	hCleanThread = CreateThread(NULL, 0, cleanThread, NULL, 0, &ulThreadId);
	if( NULL == hCleanThread)
	{
		return FALSE;
	}
	else
    {
		CloseHandle(hCleanThread);
	}
	return TRUE;
}

/**
 * receive client connection
 */
DWORD __stdcall acceptThread(void* pParam)
{
    SOCKET  sAccept;							                        //receive socket from client connection
	sockaddr_in addrClient;						                        //client SOCKET addr

	while(bConning)						                                //server's status
	{
		memset(&addrClient, 0, sizeof(sockaddr_in));					//init
		int	lenClient = sizeof(sockaddr_in);				        	//address length
		sAccept = accept(sServer, (sockaddr*)&addrClient, &lenClient);	//accept client request
		if(INVALID_SOCKET == sAccept )
		{
			int nErrCode = WSAGetLastError();
			if(nErrCode == WSAEWOULDBLOCK)	                            //cannnot finish a nonblock socket operation immediately
			{
				Sleep(TIMEFOR_THREAD_SLEEP);
				continue;                                               //continue waiting
			}
			else
            {
				return 0;                                               //thread exits
			}

		}
		else//accept request from client
			{
			    clientConn = TRUE;          //connect to client already
			    CClient *pClient = new CClient(sAccept, addrClient);
			    EnterCriticalSection(&cs);
	            //show client's IP and Ports
	            char *pClientIP = inet_ntoa(addrClient.sin_addr);
	            u_short  clientPort = ntohs(addrClient.sin_port);
	            cout<<"Accept a client IP: "<<pClientIP<<"\tPort: "<<clientPort<<endl;

				char sendBuf[50];
				sprintf_s(sendBuf, "Connect to Server successfully", inet_ntoa(addrClient.sin_addr));
				send(sAccept, sendBuf, strlen(sendBuf) + 1, 0);
			//	memcpy(dataBuf, "Connect to Server successfully", sizeof("Connect to Server successfully"));
				clientvector.push_back(pClient);            //add into vector
	            LeaveCriticalSection(&cs);

	            pClient->StartRuning();
			}
	}
	return 0;//thread exits
}

/**
 * cleanup thread resource
 */
DWORD __stdcall cleanThread(void* pParam)
 {
    while (bConning)                  //server is running
	{
		EnterCriticalSection(&cs);//enter into critical section

		//cleanup memory space of nonconnectio client
		ClIENTVECTOR::iterator iter = clientvector.begin();
		for (iter; iter != clientvector.end();)
		{
			CClient *pClient = (CClient*)*iter;
			if (!pClient->IsConning())			//client thread exit
			{
				clientvector.erase(iter);	//delete node
				delete pClient;				//release memory
				pClient = NULL;
			}else{
				iter++;						//pointer to next
			}
		}
		if(clientvector.size() == 0)
        {
            clientConn = FALSE;
        }

		LeaveCriticalSection(&cs);          //leave critical section

		Sleep(TIMEFOR_THREAD_HELP);
	}


	//server stopped
	if (!bConning)
	{
		//disconnect all connections,thread exit
		EnterCriticalSection(&cs);
		ClIENTVECTOR::iterator iter = clientvector.begin();
		for (iter; iter != clientvector.end();)
		{
			CClient *pClient = (CClient*)*iter;
			//if client connection is existence, disconnect it, thread exit
			if (pClient->IsConning())
			{
				pClient->DisConning();
			}
			++iter;
		}
		//leave critical section
		LeaveCriticalSection(&cs);

		//delay for client thread to connect, leave it exit automatically
		Sleep(TIMEFOR_THREAD_HELP);
	}

	clientvector.clear();		//cleanup linklist
	clientConn = FALSE;

	return 0;
 }

/**
 * data processing
 */
 void inputAndOutput(void)
 {
    char sendBuf[MAX_NUM_BUF];
	//CClient *sClient ;

    //showTipMsg(INPUT_DATA);

    while(bConning)
    {
    //    memset(sendBuf, 0, MAX_NUM_BUF);		//cleanup receve buffer
        cin.getline(sendBuf,MAX_NUM_BUF);	//input data
        //send data
 	//	memcpy(sendBuf, "Connect to Server successfully", sizeof("Connect to Server successfully"));
 	  //  if (clientConn)
 	    {
	//		memcpy(sendBuf, "Connect to Server successfully", sizeof("Connect to Server successfully"));
		}
	//	if(m_bReadyToSendStr)
		{
	//		memcpy(sendBuf, "Ready to send", sizeof("Ready to send"));
		}
     //   handleData(sendBuf);
    }
 }


/**
 *  select mode to handle data
 */
 void handleData(char* str)
 {
    CClient *sClient ;
    string recvsting;
    char cnum;                //show socket's number
    int num;

    if(str != NULL)
    {
      if(!strncmp(WRITE, str, strlen(WRITE))) //if cmd is WRITE
      {
      		cout <<"Connect to Server successfully"<<endl;
            EnterCriticalSection(&cs);
            str += strlen(WRITE);
            cnum = *str++;
            num = cnum - '1';
            //add vector size
            if(num < clientvector.size())
            {
                sClient = clientvector.at(num);     //send to specified client
                strcpy_s(dataBuf, str);
                sClient->IsSend();
				sClient->SetSendConnectionSuccess(TRUE);
                LeaveCriticalSection(&cs);		
            }
            else                                    //out of scope
            {
           //     cout<<"The client isn't in scope!"<<endl;
                LeaveCriticalSection(&cs);
            }

       }
        else if(!strncmp(WRITE_ALL, str, strlen(WRITE_ALL)))
        {
        //    EnterCriticalSection(&cs);
        //    str += strlen(WRITE_ALL);
		//	strcpy_s(dataBuf, str);
        //    bSend = TRUE;
		//	sClient->SetReadyToSend(TRUE);
       //     LeaveCriticalSection(&cs);
       		
      		EnterCriticalSection(&cs);
			str += strlen(WRITE);
            cnum = *str++;
            num = cnum - '1';
            //add vector size
			if(num < clientvector.size())
            {
                sClient = clientvector.at(num);     //send to specified client
                strcpy_s(dataBuf, str);
                sClient->IsSend();
				sClient->SetReadyToSend(TRUE);
                LeaveCriticalSection(&cs);			
            }
            else                                    //out of scope
            {
           //     cout<<"The client isn't in scope!"<<endl;
                LeaveCriticalSection(&cs);
            }
        }
       // else if('e'==str[0] || 'E'== str[0])     //if exit
      //  {
      //      bConning = FALSE;
      //      showServerExitMsg();
      //      Sleep(TIMEFOR_THREAD_EXIT);
      //      exitServer();
     //   }
        else
        {
           // cout <<"Input error!!"<<endl;
         }

    }
 }

/**
 *  release resource
 */
void  exitServer(void)
{
	closesocket(sServer);					//close SOCKET
	WSACleanup();							//uninstall Windows Sockets DLL
}

void showTipMsg(int input)
{
    EnterCriticalSection(&cs);
	if (START_SERVER == input)          //start server
	{
		cout << "**********************" << endl;
		cout << "* s(S): Start server *" << endl;
		cout << "* e(E): Exit  server *" << endl;
		cout << "**********************" << endl;
	//	cout << "Please input:" ;

	}
	else if(INPUT_DATA == input)
    {
        cout << "*******************************************" << endl;
        cout << "* please connect clients,then send data   *" << endl;
		cout << "* write+num+data:Send data to client-num  *" << endl;
		cout << "*   all+data:Send data to all clients     *" << endl;
		cout << "*          e(E): Exit  server             *" << endl;
		cout << "*******************************************" << endl;
		cout << "Please input:" <<endl;
    }
	 LeaveCriticalSection(&cs);
}

/**
 * show infos about server success or failure
 */
void  showServerStartMsg(BOOL bSuc)
{
	if (bSuc)
	{
		cout << "**********************" << endl;
		cout << "* Server succeeded!  *" << endl;
		cout << "**********************" << endl;
	}else{
		cout << "**********************" << endl;
		cout << "* Server failed   !  *" << endl;
		cout << "**********************" << endl;
	}

}

/**
 * show msgs about server exit
 */
void  showServerExitMsg(void)
{

	cout << "**********************" << endl;
	cout << "* Server exit...     *" << endl;
	cout << "**********************" << endl;
}

