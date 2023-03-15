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
		printf("ERROR : 원속을 초기화 할 수 없습니다.");
		return 0;
	}


	// 1. 접속 대기 소켓 생성
	/* AF_INET : L3가 IPv4 프로토콜이라는 의미
	 SOCK_STREAM : L4가 TCP가 된다는 의미
	 0 : protocol 자리인데 위의 2개로 끝나서 의미 없어서 0 */
	SOCKET hSocket = ::socket(AF_INET, SOCK_STREAM, 0);

	// 2. 포트 바인딩

	return 0;
}
