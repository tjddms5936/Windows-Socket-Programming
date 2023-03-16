#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32")

int _tmain(int argc, _TCHAR* argv[])
{
	// ���� �ʱ�ȭ
	WSADATA wsa{};
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("������ �ʱ�ȭ �� �� �����ϴ�.\n");
		return 0;
	}

	//1. ���Ӵ�� ���� ����
	SOCKET hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
	{
		printf("ERROR : ������ ������ �� �����ϴ�.\n");
		return 0;
	}

	//2. ��Ʈ ���ε� �� ����
	SOCKADDR_IN svraddr{};
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(25000);
	svraddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // �� ��ǻ�� �ڽ�
	if (::connect(hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
	{
		printf("ERROR : ������ ������ �� �����ϴ�.\n");
		return 0;
	}

	//3. ä�� �޽��� ��/����
	char szBuffer[128]{};
	while (true)
	{
		// ����ڷκ��� �Է��� �޴´�. 
		// scanf_s("%s", szBuffer, 128);
		gets_s(szBuffer);
		if (strcmp(szBuffer, "EXIT") == 0)
			break;

		// ����ڰ� �Է��� ���ڿ��� ������ �����Ѵ�.
		// int nLength{};
		// nLength = sizeof(szBuffer);
		::send(hSocket, szBuffer, strlen(szBuffer) + 1, 0); // �Է¹��� ���ڿ� �ѹ��� ����
		memset(szBuffer, 0, sizeof(szBuffer)); // szBuffer �ʱ�ȭ

		// �����κ��� ��� ���� �޼����� ���� ���� �޼����� �����Ѵ�.
		::recv(hSocket, szBuffer, sizeof(szBuffer), 0);
		printf("From Server : %s\n", szBuffer);
	}

	//4. ������ �ݰ� ����.
	::closesocket(hSocket);

	//������ ����
	::WSACleanup();
	return 0;
}