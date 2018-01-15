#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED
#include <iostream>
#include <winsock2.h>
#include <winbase.h>
#include <vector>
#include <map>
#include <string>
#include <process.h>
#include "sclient.h"

using namespace std;
#pragma comment(lib, "ws2_32.lib")			//dynamic function

/**
 * macro define
 */
#define SERVER_SETUP_FAIL			1		//start client failure
#define START_SERVER                1       //show reminder of begining to input
#define INPUT_DATA                  2       //remind input what data

#define SERVERPORT			6006			//server TCP port
#define CONN_NUM            10              //connection number to connection client
#define TIMEFOR_THREAD_SLEEP		500		//�ȴ��ͻ��������߳�˯��ʱ��
#define TIMEFOR_THREAD_HELP			1500	//������Դ�߳��˳�ʱ��
#define TIMEFOR_THREAD_EXIT			5000	//���߳�˯��ʱ��
#define WRITE_ALL  "Ready to send"// "all"                   //���������ݷ�������
#define WRITE      "Connect to Server successfully"// "write"                 //���ͱ�־
#define READ       "read"                  //������ʾ��־
#define READ_ALL    "read all"              //�������пͻ�������
typedef vector<CClient*> ClIENTVECTOR;		//��������
typedef vector<string> SVECTOR;             //�����ַ�

/**
 * ȫ�ֱ���
 */
extern char	dataBuf[MAX_NUM_BUF];				//д������
extern BOOL	bConning;							//��ͻ��˵�����״̬
extern BOOL    bSend;                              //���ͱ��λ
extern BOOL    clientConn;                         //���ӿͻ��˱��
extern SOCKET	sServer;							//�����������׽���
extern CRITICAL_SECTION  cs;			            //�������ݵ��ٽ�������
extern HANDLE	hAcceptThread;						//���ݴ����߳̾��
extern HANDLE	hCleanThread;						//���ݽ����߳�
extern BOOL m_bReadyToSendStr;
/**
 *��������
 */
BOOL initSever(void);                       //��ʼ��
void initMember(void);
bool initSocket(void);						//��ʼ���������׽���
void exitServer(void);						//�ͷ���Դ
bool startService(void);					//����������
void inputAndOutput(void);                  //��������
void showServerStartMsg(BOOL bSuc);         //��ʾ������Ϣ
void showServerExitMsg(void);               //��ʾ�˳���Ϣ
void handleData(char* str);                 //���ݴ���
void showTipMsg(BOOL bFirstInput);          //��ʾ������ʾ��Ϣ
BOOL createCleanAndAcceptThread(void);      //������غ���
DWORD __stdcall acceptThread(void* pParam); //�����ͻ��������߳�
DWORD __stdcall cleanThread(void* pParam);


#endif // SERVER_H_INCLUDED
