#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32")

int _tmain(int argc, _TCHAR* argv[])
{
	// 원속 초기화
	WSADATA wsa{};
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("원속을 초기화 할 수 없습니다.\n");
		return 0;
	}

	//1. 접속대기 소켓 생성
	SOCKET hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
	{
		printf("ERROR : 소켓을 생성할 수 없습니다.\n");
		return 0;
	}

	//2. 포트 바인딩 및 연결
	SOCKADDR_IN svraddr{};
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(25000);
	svraddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // 내 컴퓨터 자신
	if (::connect(hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
	{
		printf("ERROR : 서버에 연결할 수 없습니다.\n");
		return 0;
	}

	//3. 채팅 메시지 송/수신
	char szBuffer[128]{};
	while (true)
	{
		// 사용자로부터 입력을 받는다. 
		// scanf_s("%s", szBuffer, 128);
		gets_s(szBuffer);
		if (strcmp(szBuffer, "EXIT") == 0)
			break;

		// 사용자가 입력한 문자열을 서버에 전송한다.
		// int nLength{};
		// nLength = sizeof(szBuffer);
		::send(hSocket, szBuffer, strlen(szBuffer) + 1, 0); // 입력받은 문자열 한번에 전송
		memset(szBuffer, 0, sizeof(szBuffer)); // szBuffer 초기화

		// 서버로부터 방금 보낸 메세지에 대한 에코 메세지를 수신한다.
		::recv(hSocket, szBuffer, sizeof(szBuffer), 0);
		printf("From Server : %s\n", szBuffer);
	}

	//4. 소켓을 닫고 종료.
	::closesocket(hSocket);

	//※윈속 해제
	::WSACleanup();
	return 0;
}