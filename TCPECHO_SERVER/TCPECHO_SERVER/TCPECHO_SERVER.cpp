// TCPECHO_SERVER.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "stdafx.h"
#include <WinSock2.h>
#pragma comment(lib, "ws2_32")
/*
아니.. int arData[]가 곧 int* arData 랑 같은거잖아..
근데 이거는 int형 자료형의 배열의 주소를 넘겨주는 건데
int* arData[] 는 뭐야?;;

배열을 넘기고 싶으면 포인터를 써서 넘겨주던가, []를 붙여서 넘겨주면 된느데
포인터하고 [] 둘 다 쓰는건 뭐지?;;
*/
int _tmain(int argc, _TCHAR* argv[])
{
	// 원속 초기화 
	WSADATA wsa{};
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("ERROR : 원속을 초기화 할 수 없습니다.\n");
		return 0;
	}


	// 1. 접속 대기 소켓 생성
	/* AF_INET : L3가 IPv4 프로토콜이라는 의미
	 SOCK_STREAM : L4가 TCP가 된다는 의미
	 0 : protocol 자리인데 위의 2개로 끝나서 의미 없어서 0 */
	SOCKET hSocket = ::socket(AF_INET, SOCK_STREAM, 0);

	// 2. 포트 바인딩
	SOCKADDR_IN svraddr{};
	svraddr.sin_family = AF_INET; // IPv4
	svraddr.sin_port = htons(25000); // 리틀엔디안을 빅엔디안으로 바꿔주는 작업
	svraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY); //  IP주소가 뭐가 되든지 신경쓰지 않겠다.
	if (::bind(hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
	{
		printf("ERROR : 소켓에 IP주소와 포트를 바인딩 할 수 없습니다.\n");
		return 0;
	}

	// 3. 접속 대기 상태(listen)으로 전환
	if (::listen(hSocket, SOMAXCONN) == SOCKET_ERROR) 
		// SOMAXCONN : 대기처리를 OS에서 다 해주기 때문에 OS레벨에서 할 수 있는만큼 알아서 하라는 의미
	{
		printf("ERROR : 리슨 상태로 전환할 수 없습니다.\n");
		return 0;
	}

	// 4. 클라이언트 접속 처리 및 대응 
	SOCKADDR_IN clientaddr{};
	SOCKET hClient{};
	int nAddrLen{};
	char szBuffer[128]{};
	int	nReceive{};

	nAddrLen = sizeof(clientaddr);
	// 4-1. 클라이언트 연결을 받아들이고 새로운 소켓 생성(개방)
	while (hClient = ::accept(hSocket, 
		(SOCKADDR*)&clientaddr, 
		&nAddrLen) != INVALID_SOCKET)
	{
		printf("새 클라이언트가 연결되었습니다.\n");
		// 4-2. 클라이언트로부터 문자열을 수신함
		while (nReceive = ::recv(hClient, szBuffer, sizeof(szBuffer), 0) > 0)
		{
			// 4-3. 수신한 문자열을 그대로 반향전송
			::send(hClient, szBuffer, sizeof(szBuffer), 0); // 클라이언트에게 보내주고
			printf("%s\n", szBuffer); // 서버쪽 화면에 한번 찍고
			fflush(stdout); // ?? 
			memset(szBuffer, 0, sizeof(szBuffer)); // szBuffer 초기화
		}

		// 클라이언트가 연결을 종료함 
		shutdown(hClient, SD_BOTH);
		closesocket(hClient);
		printf("클라이언트와의 연결이 끊어졌습니다.");
	}
	printf("클라이언트와의 연결이 없음.");
	// 5. 리슨 소켓 닫기
	::closesocket(hSocket);

	// 원속 해제
	::WSACleanup();


	return 0;
}
