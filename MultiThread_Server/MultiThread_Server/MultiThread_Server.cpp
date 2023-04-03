#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32")
#include <windows.h>
#include <list>
#include <iterator>

// 전역변수 선언
CRITICAL_SECTION g_cs; // 스레드 동기화 객체
SOCKET g_hSocket; // 서버의 리슨 소켓
std::list<SOCKET> g_listClient; // 연결된 클라이언트 소켓 리스트

BOOL AddUser(SOCKET hSocket);
void SentChattingMessage(char* pszParam);
BOOL CtrlHandler(DWORD dwType);
DWORD WINAPI ThreadFunction(LPVOID pParam);


int _tmain(int argc, _TCHAR* argv[])
{
	// 윈속 초기화
	WSADATA wsa{};
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		puts("ERROR : 윈속을 초기화할 수 없습니다.\n");
		return 0;
	}

	// 임계영역 객체 생성
	::InitializeCriticalSection(&g_cs);

	// Ctrl + C 이벤트 감지 및 처리 함수 등록
	if (::SetConsoleCtrlHandler(
		(PHANDLER_ROUTINE)CtrlHandler, TRUE) == FALSE
		)
		puts("ERROR: Ctrl+C 처리기를 등록할 수 없습니다.");

	// 1. 접속 대기 소켓 생성
	g_hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (g_hSocket == INVALID_SOCKET)
	{
		puts("ERROR : 접속 대기 소켓을 생성할 수 없습니다.\n");
		return 0;
	}

	// 2. 포트 바인딩
	SOCKADDR_IN svraddr{};
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(25000);
	svraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(
		g_hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR
		)
	{
		puts("ERROR: 소켓에 IP주소와 포트를 바인드 할 수 없습니다.");
		return 0;
	}

	// 3. Listen 상태로 전환(접속 대기 상태)
	if (::listen(g_hSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		puts("ERROR: 리슨 상태로 전환할 수 없습니다.");
		return 0;
	}
	puts("*** 채팅서버를 시작합니다. ***");

	// 4-1. 클라이언트 접속 처리 및 대응
	SOCKADDR_IN clientaddr{};
	int nAddrLength{};
	nAddrLength = sizeof(clientaddr);
	SOCKET hClient{};
	DWORD dwThreadID{};
	HANDLE hThread{};

	// 4-2. 클라이언트 연결을 받아들이고 새로운 소켓 개방
	while ((hClient = ::accept(g_hSocket,
		(SOCKADDR*)&clientaddr, &nAddrLength)) != INVALID_SOCKET)
	{
		if (AddUser(hClient) == FALSE)
		{
			puts("더이상 클라이언트 연결을 처리할 수 없습니다.\n");
			CtrlHandler(CTRL_C_EVENT); // Ctrl + C 입력 했다고 처리하기
			break;
		}
		// 클라이언트 소켓이 리스트에 잘 추가됨
		// 클라이언트로부터 문자열을 수신함.
		hThread = CreateThread(
			NULL,
			0,
			ThreadFunction,
			(LPVOID)hClient,
			0,
			&dwThreadID);

		::CloseHandle(hThread);
	}

	// 위의 while 작업이 끝난 상태라면, 이미 서버가 연결을 끊은 상황이고, 필요한 작업도 Ctrl + C 이벤트 처리 함수에서 했다.
	puts("*** 채팅 서버를 종료합니다. ***\n");
	return 0;
}

BOOL AddUser(SOCKET hSocket)
{
	::EnterCriticalSection(&g_cs); // 임계 구간 진입 : Lock()
	// 임계 구간 안에서는 오직 한 스레드만 수행하는 것이 보장된다.
	g_listClient.push_back(hSocket);
	::LeaveCriticalSection(&g_cs); // 임계 구간 종료 : UnLock()
	return TRUE;
}

void SentChattingMessage(char* pszParam)
{
	int nLength{};
	nLength = strlen(pszParam);
	std::list<SOCKET>::iterator it;

	::EnterCriticalSection(&g_cs); // 임계 구간 진입 : Lock()
	for (it = g_listClient.begin(); it != g_listClient.end(); it++)
		::send(*it, pszParam, sizeof(char) * (nLength+1), 0);
	::LeaveCriticalSection(&g_cs); // 임계 구간 종료 : UnLock()
}

BOOL CtrlHandler(DWORD dwType)
{
	//Ctrl+C 이벤트를 감지하고 프로그램을 종료한다.
	if (dwType == CTRL_C_EVENT)
	{
		std::list<SOCKET>::iterator it;

		// 리슨 소켓과 연결된 모든 클라이언트 소켓을 닫고 프로그램을 종료한다. 
		::shutdown(g_hSocket, SD_BOTH); // 서버 소켓 닫기
		puts("서버 소켓을 닫았습니다.\n");
		::EnterCriticalSection(&g_cs); // 임계 구간 진입 : Lock()
		for (it = g_listClient.begin(); it != g_listClient.end(); it++)
			::closesocket(*it);
		g_listClient.clear(); 
		::LeaveCriticalSection(&g_cs); // 임계 구간 종료 : UnLock()

		puts("모든 클라이언트 연결을 종료했습니다.\n");

		// 클라이언트와 통신하는 스레드들이 종료되기를 기다린다. 
		Sleep(100); // 100ms면 다 종료되었겠거니 '추정;
		::DeleteCriticalSection(&g_cs);
		::closesocket(g_hSocket);

		// 윈속 해제
		::WSACleanup();
		exit(0);
		return TRUE;
	}

	return FALSE;
}

DWORD __stdcall ThreadFunction(LPVOID pParam)
{
	//클라이언트에게 채팅 메시지 서비스를 제공하는 스레드 함수.
	//연결된 각각의 클라이언트마다 한 스레드가 생성된다.
	char szBuffer[128]{};
	int nReceive{};
	SOCKET hClient{};
	hClient = (SOCKET)pParam;

	puts("새 클라이언트가 연결되었습니다.\n");
	while (
		(nReceive = ::recv(hClient, szBuffer, sizeof(szBuffer), 0)) > 0
		)
	{
		puts(szBuffer);
		// 수신한 문자열을 다른 모든 클라이언트들에게 전송
		SentChattingMessage(szBuffer);
		memset(szBuffer, 0, sizeof(szBuffer));
	}

	// while문 종료라는 것은? 이 Thread와 연결된 클라이언트와 연결이 종료되었다는 것
	puts("클라이언트가 연결을 끊었습니다.\n");
	::EnterCriticalSection(&g_cs); // 임계 구간 진입 : Lock()
	g_listClient.remove(hClient);
	::LeaveCriticalSection(&g_cs); // 임계 구간 종료 : UnLock()

	closesocket(hClient);
	return 0;
}
