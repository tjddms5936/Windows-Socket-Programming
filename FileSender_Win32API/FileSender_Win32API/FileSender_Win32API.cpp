// FileSender_Win32API.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32")
#include <Mswsock.h> //::TranmitFile() 함수를 사용하기 위한 헤더와 라이브러리 설정
#pragma comment(lib, "Mswsock")

// 전송할 파일에 대한 정보를 담기위한 구조체
struct MY_FILE_DATA
{
	char szName[_MAX_FNAME];
	DWORD dwSize;
};

void ErrorHandler(const char* pszMessage);

int _tmain(int argc, _TCHAR* argv[])
{
	// 윈속 초기화
	WSADATA wsa{};
	if (::WSAStartup(MAKEWORD(2, 2), &wsa))
		ErrorHandler("윈속을 초기화 할 수 없습니다.");

	// 전송할 파일 개방
	HANDLE hFile = ::CreateFile(_T("Sleep Away.zip"),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		ErrorHandler("전송할 파일을 개방할 수 없습니다.");

	// 1. 접속 대기 소켓 생성
	SOCKET hSocket{};
	hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if(hSocket == INVALID_SOCKET)
		ErrorHandler("접속 대기 소켓을 생성할 수 없습니다.");

	// 2. 포트 바인딩
	SOCKADDR_IN svraddr{};
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(25000);
	svraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if(::bind(hSocket, (SOCKADDR*)& svraddr, sizeof(svraddr)) == SOCKET_ERROR)
		ErrorHandler("소켓에 IP주소와 포트를 바인드 할 수 없습니다.");

	// 3. 접속 대기 상태로 전환
	if(::listen(hSocket, SOMAXCONN) == SOCKET_ERROR)
		ErrorHandler("리슨 상태로 전환할 수 없습니다.");

	puts("파일송신서버를 시작합니다.");

	// 4. 클라이언트 연결을 받아들이고 새로운 소켓 개방
	SOCKADDR_IN clientaddr{};
	int nAddrLen{};
	SOCKET hClient{};

	nAddrLen = sizeof(clientaddr);
	hClient = ::accept(hSocket, (SOCKADDR*)&clientaddr, &nAddrLen);

	if(hClient == INVALID_SOCKET)
		ErrorHandler("클라이언트 통신 소켓을 생성할 수 없습니다.");

	puts("클라이언트가 연결되었습니다.");

	// 5. 전송할 파일에 대한 정보를 작성한다.
	MY_FILE_DATA fData = { "Sleep Away.zip", 0 };
	fData.dwSize = ::GetFileSize(hFile, NULL);
	TRANSMIT_FILE_BUFFERS tfb{};
	tfb.Head = &fData;
	tfb.HeadLength = sizeof(fData);

	// 6. 파일 송신
	if (::TransmitFile(
		hClient,	//파일을 전송할 소켓 핸들.
		hFile,		//전송할 파일 핸들.
		0,			//전송할 크기. 0이면 전체.
		65536,		//한 번에 전송할 버퍼 크기.
		NULL,		//비동기 입/출력에 대한 OVERLAPPED 구조체.
		&tfb,		//파일 전송에 앞서 먼저 전송할 데이터.
		0			//기타 옵션.
	) == FALSE)
		ErrorHandler("파일을 전송할 수 없습니다.");

	// 클라이언트가 연결 끊는것을 대기한다.
	::recv(hClient, NULL, 0, 0);
	puts("클라이언트가 연결을 끊었습니다.");

	::closesocket(hClient);
	::closesocket(hSocket);
	::CloseHandle(hFile);
	::WSACleanup();
	return 0;
}

void ErrorHandler(const char* pszMessage)
{
	printf("ERROR : %s\n", pszMessage);
	::WSACleanup();
	exit(1);
}
