// ServerMonitor.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "server.h"
int main(int argc, char* argv[])
{
	//init Server
	if (!initSever())
	{
		exitServer();
		return SERVER_SETUP_FAIL;
	}

	//start service
	if (!startService())
	{
		showServerStartMsg(FALSE);
		exitServer();
		//return SERVER_SETUP_FAIL;
	}

	//handle data
	inputAndOutput();
	
	//exit main thread, cleanup resource
//	exitServer();

	return 0;
}

