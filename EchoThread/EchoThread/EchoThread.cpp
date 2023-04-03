// EchoThread.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "stdafx.h"
#include <WinSock2.h>
#pragma comment(lib, "ws2_32") 

//연결된 클라이언트와 통신(Echo 서비스)하기 위한 작업자 스레드 함수.
DWORD WINAPI ThreadFunction(LPVOID pParam);

int _tmain(int argc, _TCHAR* argv[])
{
	// 윈속 초기화
	WSADATA wsa{};
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("ERROR : 윈속을 초기화 할 수 없습니다.\n");
		return 0;
	}

	// 1. 접속대기 소켓 생성
	SOCKET hSocket{};
	hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
	{
		printf("ERROR : 접속 대기 소켓을 생성할 수 없습니다.\n");
		return 0;
	}

	// 2. 포트 바인딩
	SOCKADDR_IN svraddr{};
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(25000);
	svraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hSocket,
		(SOCKADDR*)&svraddr,
		sizeof(svraddr)) == SOCKET_ERROR)
	{
		printf("ERROR : 소켓에 IP주소와 포트를 바인딩할 수 없습니다.\n");
		return 0;
	}

	// 3. 접속 대기 상태로 전환
	if (::listen(hSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("ERROR : 리슨 상태로 전환할 수 없습니다.\n");
		return 0;
	}

	// 4. 클라이언트 접속 처리 및 대응
	SOCKADDR_IN clientaddr{};
	int nAddrLength{};
	nAddrLength = sizeof(clientaddr);
	SOCKET hClient{};
	DWORD dwThreadID{};
	HANDLE hThread{};

	// 4-2. 클라이언트 연결을 받아들이고 새로운 소켓 개방
	while (
		(hClient = 
			::accept(hSocket, (SOCKADDR*)&clientaddr, &nAddrLength)
			) != INVALID_SOCKET)
	{
		// 4-3. 새 클라이언트와 통신하기 위해 스레드 생성
		// 클라이언트마다 스레드가 하나씩 생성된다.
		hThread = CreateThread(
			NULL, // 보안속성 상속
			0, // 스택메모리는 기본 크기
			ThreadFunction, // 스레드로 실행할 함수 이름
			(LPVOID)hClient, // 새로 생성된 클라이언트 소켓
			0, // 생성 플래그는 기본값 사용
			&dwThreadID); // 생성된 스레드 ID가 저장될 변수 주소 

		::CloseHandle(hThread); 
	}

	// 5. 리슨 소켓 닫기
	closesocket(hSocket);

	// 윈속 해제
	::WSACleanup();
	return 0;
}

DWORD __stdcall ThreadFunction(LPVOID pParam)
{
	char szBuffer[128]{};
	int nRecieve{};
	SOCKET hClient{};
	
	hClient = (SOCKET)pParam;

	puts("새 클라이언트가 연결되었습니다.");

	// 클라이언트로부터 문자열을 수신한다. 
	while (
		(nRecieve = ::recv(hClient, szBuffer, sizeof(szBuffer), 0)) > 0
		)
	{
		// 수신한 문자열을 그대로 반향 전송
		::send(hClient, szBuffer, sizeof(szBuffer), 0);
		puts(szBuffer);
		memset(szBuffer, 0, sizeof(szBuffer));
	}

	puts("클라이언트 연결이 끊겼습니다.");
	::closesocket(hClient);
	return 0;
}
