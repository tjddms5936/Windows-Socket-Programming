// EventSelectServer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32")
#include <windows.h>
#include <list>
#include <iterator>

void CloseAll();
BOOL CtrlHandler(DWORD dwType);

SOCKET g_hSocket;
//배열요소에 대한 인덱스. 이 값은 WSA_MAXIMUM_WAIT_EVENTS보다 작다.
int g_nListIndex;
// 소켓에 대한 이벤트 핸들의 배열
WSAEVENT g_aListEvent[WSA_MAXIMUM_WAIT_EVENTS];
// 소켓의 배열. 이벤트 핸들 배열과 매핑
SOCKET g_ListSocket[WSA_MAXIMUM_WAIT_EVENTS];

int _tmain(int argc, _TCHAR* argv[])
{
	// 윈속 초기화
	WSADATA wsa{};
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		puts("ERROR: 윈속을 초기화 할 수 없습니다.");
		return 0;
	}

	//Ctrl+C 키를 눌렀을 때 이를 감지하고 처리할 함수를 등록한다.
	if (::SetConsoleCtrlHandler(
		(PHANDLER_ROUTINE)CtrlHandler, TRUE) == FALSE)
		puts("ERROR: Ctrl+C 처리기를 등록할 수 없습니다.");

	// 1. 접속 대기 소켓 생성
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

	// 3. listen 전환 (접속대기)
	if (::listen(g_hSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		puts("ERROR: 리슨 상태로 전환할 수 없습니다.");
		return 0;
	}

	// 4. 통지 이벤트 수신을 위한 소켓 이벤트 등록.
	g_nListIndex = 0;
	g_ListSocket[g_nListIndex] = g_hSocket;
	g_aListEvent[g_nListIndex] = ::WSACreateEvent();
	// 서버 소켓이므로 FD_ACCEPT 이벤트를 감시한다.
	if (::WSAEventSelect(g_ListSocket[g_nListIndex], g_aListEvent[g_nListIndex], FD_ACCEPT) == SOCKET_ERROR)
	{
		puts("ERROR: WSAEventSelect()");
		return 0;
	}
	puts("*** 서버를 시작합니다. ***");

	// 5. 루프를 돌면서 이벤트를 처리한다.
	DWORD dwIndex{};
	WSANETWORKEVENTS netEenvt{};
	while (TRUE)
	{
		// 소켓 이벤트를 기다린다.
		dwIndex = ::WSAWaitForMultipleEvents(
			g_nListIndex + 1, // 감시할 이벤트 개수
			g_aListEvent, // 이벤트 배열
			FALSE, // 전체에 대해 대기하지 않는다.
			100, // 100ms 동안 대기한다. 
			FALSE); // 호출자 스레드 상태를 변경하지 않는다.

		if (dwIndex == WSA_WAIT_FAILED)
			continue;

		// 이벤트가 발생한 소켓의 인덱스 + 이벤트 발생 이유를 확인
		if (::WSAEnumNetworkEvents(g_ListSocket[dwIndex], g_aListEvent[dwIndex], &netEenvt) == SOCKET_ERROR)
			continue;

		// 5-1. 클라이언트가 연결함
		if (netEenvt.lNetworkEvents & FD_ACCEPT)
		{
			if (netEenvt.iErrorCode[FD_ACCEPT_BIT] != 0)
				continue;

			if (g_nListIndex >= WSA_MAXIMUM_WAIT_EVENTS)
			{
				puts("ERROR: 최대 접속상태!");
				continue;
			}

			SOCKADDR_IN clientaddr{};
			int nAddrLen{};
			SOCKET hClient{};

			nAddrLen = sizeof(clientaddr);
			hClient = ::accept(g_hSocket, (SOCKADDR*)&clientaddr, &nAddrLen);
			if (hClient != INVALID_SOCKET)
			{
				// 새 클라이언트 통시 소켓과 이벤트 핸들을 등록
				g_nListIndex++;
				g_ListSocket[g_nListIndex] = hClient;
				g_aListEvent[g_nListIndex] = ::WSACreateEvent();
				puts("새 클라이언트가 연결됐습니다.");
			}

			// 클라이언트 소켓에 대한 읽기, 닫기 이벤트를 감시한다.
			::WSAEventSelect(hClient, g_aListEvent[g_nListIndex],
				FD_READ | FD_CLOSE);


		}
		// 5-2. 클라이언트가 연결을 끊음
		else if (netEenvt.lNetworkEvents & FD_CLOSE)
		{
			// 클라이언트 소켓 및 이벤트 핸들을 닫는다.
			::WSACloseEvent(g_aListEvent[dwIndex]);
			::shutdown(g_ListSocket[dwIndex], SD_BOTH);
			::closesocket(g_ListSocket[dwIndex]);

			//닫은 소켓 및 핸들정보를 삭제하고 배열을 정리한다.
			for (DWORD i = dwIndex; i < g_nListIndex; i++)
			{
				g_aListEvent[i] = g_aListEvent[i + 1];
				g_ListSocket[i] = g_ListSocket[i + 1];
			}
			g_nListIndex--;
			printf("클라이언트 연결 종료. 남은 수: %d.\n", g_nListIndex);
		}

		// 5-3. 클라이언트가 데이터를 전송함
		else if (netEenvt.lNetworkEvents & FD_READ)
		{
			TCHAR szBuffer[1024]{};
			int nRecieve{};

			nRecieve = ::recv(g_ListSocket[dwIndex], (char*)szBuffer, sizeof(szBuffer), 0);

			//연결된 모든 클라이언트에 같은 메시지를 전송한다.
			for (DWORD i = 1; i < g_nListIndex; i++)
			{
				::send(g_ListSocket[i], (char*)szBuffer, nRecieve, 0);
			}
		}
	}



	return 0;
}

void CloseAll()
{
	// 서버 소켓 + 연결된 클라이언트 연결을 종료함
	for (int i = 0; i < g_nListIndex; i++)
	{
		::shutdown(g_ListSocket[i], SD_BOTH);
		::closesocket(g_ListSocket[i]);
		::WSACloseEvent(g_aListEvent[i]);
	}
	puts("모든 클라이언트 연결을 종료했습니다.");
}

BOOL CtrlHandler(DWORD dwType)
{
	if (dwType == CTRL_C_EVENT)
	{
		CloseAll();
		::WSACleanup();
		exit(0);
		return TRUE;
	}

	return FALSE;
}
