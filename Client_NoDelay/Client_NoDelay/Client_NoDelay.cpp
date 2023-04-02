// Client_NoDelay.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32")

int _tmain(int argc, _TCHAR* arvg[])
{
	// 윈속 초기화
	WSADATA wsa{};
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("ERROR : 윈속을 초기화할 수 없습니다.");
		return 0;
	}

	// 1. 접속 대기 소켓 생성
	SOCKET hSocket{};
	hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
	{
		printf("ERROR : 소켓을 생성할 수 없습니다.");
		return 0;
	}

	// 2. 포트 바인딩 연결
	SOCKADDR_IN svraddr{};
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(25000);
	svraddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (::connect(hSocket,
		(SOCKADDR*)&svraddr,
		sizeof(svraddr)) == SOCKET_ERROR)
	{
		printf("ERROR : 서버에 연결할 수 없습니다.");
		return 0;
	}

	// 2.1 NoDelay 설정
	int nOpt{};
	nOpt = 1;
	::setsockopt(hSocket, IPPROTO_TCP, TCP_NODELAY,
		(char*)&nOpt, sizeof(nOpt));

	// 3. 채팅 메시지 송/수신
	char szBuffer[128]{};
	while (1)
	{
		// 사용자로부터 문자열을 입력 받는다.
		scanf_s("%s", szBuffer, 128);
		if (strcmp(szBuffer, "EXIT") == 0)
			break;

		// 사용자가 입력한 문자열을 한번에 서버에 전송. null문자열 포함
		// ::send(hSocket, szBuffer, strlen(szBuffer) + 1, 0);

		// 사용자가 입력한 문자열을 하나씩 서버에 전송
		int nLength{};
		nLength = strlen(szBuffer);
		for (int i = 0; i < nLength; i++)
		{
			::send(hSocket, szBuffer + i, 1, 0);
		}

		// 서버로부터 방금 보낸 에코 메세지 수신
		memset(szBuffer, 0, sizeof(szBuffer));
		::recv(hSocket, szBuffer, sizeof(szBuffer), 0);
		printf("From Server : %s\n", szBuffer);
	}

	// 4. 소켓을 닫고 종료
	::shutdown(hSocket, SD_BOTH);
	::closesocket(hSocket);

	// 윈속 해제
	WSACleanup();
	return 0;
}
