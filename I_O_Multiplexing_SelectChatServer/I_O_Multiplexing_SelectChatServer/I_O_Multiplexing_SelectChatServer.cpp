// I_O_Multiplexing_SelectChatServer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32")
#include <windows.h>
#include <list>
#include <iterator>

BOOL CtrlHandler(DWORD dwType);
void CloseAll();
void SendMessageAll(char* pszBuffer, int nSize);

SOCKET g_hSocket; // 서버의 리슨 소켓
std::list<SOCKET> g_listClient; // 연결된 클라이언트 소켓 리스트

int _tmain(int argc_, _TCHAR* argv[])
{
	// 윈속 초기화 
	WSADATA wsa{};
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		puts("ERROR: 윈속을 초기화 할 수 없습니다.");
		return 0;
	}

	//Ctrl+C 키를 눌렀을 때 이를 감지하고 처리할 함수를 등록한다.
	if(::SetConsoleCtrlHandler(
	(PHANDLER_ROUTINE)CtrlHandler, TRUE) == FALSE)
		puts("ERROR: Ctrl+C 처리기를 등록할 수 없습니다.");

	// 1. 접속대기 소켓 생성
	g_hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (g_hSocket == INVALID_SOCKET)
	{
		puts("ERROR: 접속 대기 소켓을 생성할 수 없습니다.");
		return 0;
	}

	// 2. 포트 바인딩
	SOCKADDR_IN svraddr{};
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(25000);
	svraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(g_hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
	{
		puts("ERROR: 소켓에 주소를 바인드 할 수 없습니다.");
		return 0;
	}

	// 3. 접속대기 g_hSocket 전환 
	if (::listen(g_hSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		puts("ERROR: 리슨 상태로 전환할 수 없습니다.");
		return 0;
	}

	// 4. 연결된 클라이언트 관리를 위한 리스트 컨테이너 생성
	// ﻿연결된 클라이언트 뿐만 아니라 서버 소켓도 집어넣어서 같이 감시를 한방에 하겠다 라는 의도이다.
	g_listClient.push_back(g_hSocket);

	// 5. 소켓의 변화를 감시를 위한 반복문
	UINT nCount{};
	FD_SET fdRead{};
	std::list<SOCKET>::iterator it;

	puts("I/O 멀티플렉싱 채팅 서버를 시작합니다.");
	while (g_hSocket != NULL)
	{
		// 5-1. 클라이언트 접속 및 정보수신 변화 감시셋 초기화
		
		// FD_ZERO : 0으로 배열 초기화를 하고 
		FD_ZERO(&fdRead); 

		for (it = g_listClient.begin(); it != g_listClient.end(); it++)
			// 클라이언트 연결이 무언가 되었다면 연결된 소켓에서 변화가 있는지 handle들에 대해서 조사를 한다.
			FD_SET(*it, &fdRead);

		//5-2. 변화가 발생할 때까지 대기
		::select(0, &fdRead, NULL, NULL, NULL);

		//5-3. 변화가 감지된 소켓 셋 확인
		nCount = fdRead.fd_count;
		for (int nIndex = 0; nIndex < nCount; nIndex++)
		{
			//소켓에 변화 감시 플래그가 세트 되었는가?
			if (!FD_ISSET(fdRead.fd_array[nIndex], &fdRead))
				continue;

			//5-3-1. 서버의 listen 소켓이 세트되었는가? 
			//즉, 누군가 연결을 시도했는가?
			if (fdRead.fd_array[nIndex] == g_hSocket)
			{
				//새 클라이언트의 접속을 받는다.
				SOCKADDR_IN clientAddr{};
				int nAddrLen = sizeof(clientAddr);
				SOCKET hClient{};
				hClient = ::accept(g_hSocket, (SOCKADDR*)&clientAddr, &nAddrLen);

				if (hClient != INVALID_SOCKET)
				{
					FD_SET(hClient, &fdRead);
					g_listClient.push_back(hClient);
				}
			}
			//5-3-2 클라이언트가 전송한 데이터가 있는 경우.
			else
			{
				char szBuffer[1024]{};
				int nReceive{};
				nReceive = ::recv(fdRead.fd_array[nIndex], (char*)szBuffer, sizeof(szBuffer), 0);
				if (nReceive <= 0)
				{
					// 연결 종료
					::closesocket(fdRead.fd_array[nIndex]);
					FD_CLR(fdRead.fd_array[nIndex], &fdRead);
					g_listClient.remove(fdRead.fd_array[nIndex]);
					puts("클라이언트가 연결을 끊었습니다.");
				}
				else
				{
					//채팅 메시지 전송
					SendMessageAll(szBuffer, nReceive);
				}
			}
		}
	}

	//6. 모든 연결을 닫고 종료한다.
	CloseAll();
	puts("서버를 종료합니다.");

	return 0;
}

BOOL CtrlHandler(DWORD dwType)
{
	if (dwType == CTRL_C_EVENT)
	{
		::closesocket(g_hSocket);
		CloseAll();
		puts("모든 클라이언트 연결을 종료했습니다.");

		::WSACleanup();
		exit(0);
		return TRUE;
	}

	return FALSE;
}

void CloseAll()
{
	std::list<SOCKET>::iterator it;
	
	for (it = g_listClient.begin(); it != g_listClient.end(); it++)
	{
		::shutdown(*it, SD_BOTH);
		::closesocket(*it);
	}
}

void SendMessageAll(char* pszBuffer, int nSize)
{
	std::list<SOCKET>::iterator it;
	for (it = g_listClient.begin(); it != g_listClient.end(); it++)
	{
		send(*it, pszBuffer, nSize, 0);
	}
}
