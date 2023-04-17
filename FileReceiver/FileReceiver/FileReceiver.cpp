// FileReceiver.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32")

void ErrorHandler(const char* pszMessage);

int _tmain(int argc, _TCHAR* argv[])
{
	// 윈속 초기화
	WSADATA wsa{};
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		ErrorHandler("윈속을 초기화 할 수 없습니다.");

	// 1. 소켓 생성
	SOCKET hSocket{};
	hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
		ErrorHandler("소켓을 생성할 수 없습니다.");

	// 2. 포트 바인딩 및 연결
	SOCKADDR_IN svraddr{};
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(25000);
	svraddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (::connect(hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
		ErrorHandler("서버에 연결할 수 없습니다.");

	// 3. 수신할 파일을 생성한다.
	puts("*** 파일 수신을 시작합니다.***");
	FILE* fp{};
	errno_t nResult{};
	nResult = fopen_s(&fp, "Sleep away.zip", "wb");
	if (nResult != 0)
		ErrorHandler("파일을 생성 할 수 없습니다.");

	// 4. 서버가 전송하는 데이터를 반복해 파일에 붙여 넣는다.
	char byBuffer[65536]{};
	int nRecv{};

	while (nRecv = ::recv(hSocket, byBuffer, 65536, 0) > 0)
	{
		// 서버에서 받은 크기만큼 데이터를 파일에 쓴다. 
		fwrite(byBuffer, nRecv, 1, fp);
		putchar('#');
	}

	fclose(fp);
	puts("\n*** 파일수신이 끝났습니다. ***\n");

	// 소켓을 닫고 프로그램 종료
	// 연결을 먼저 끊는 것은 '서버'이다. 좋지 않은 구조이긴 하나 예제일뿐
	::closesocket(hSocket);
	::WSACleanup();
	return 0;
}

void ErrorHandler(const char* pszMessage)
{
	printf("ERROR : %s\n", pszMessage);
	::WSACleanup();
	exit(1);
}
