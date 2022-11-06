#include "Common.h"
#include <conio.h>	// ���� Ű���� �Է� ������ ���� kbhit()�Լ� ����� ���� Header

#define SERVERPORT 9000
#define BUFSIZE    512
#define MAXCLIENTS 20		// �ϴ� 20��

// ������ Ŭ���̾�Ʈ ���
SOCKET sockets[MAXCLIENTS];
int num = 0;	// �׻� ������ ������ Ŭ���̾�Ʈ ����

// Ŭ���̾�Ʈ�� ������ ���
DWORD WINAPI ProcessClient(LPVOID arg)
{
	int retval;
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	char buf[BUFSIZE + 1];

	// Ŭ���̾�Ʈ ���� ���
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr *)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	while (1) {
		// ������ �ޱ�
		retval = recv(client_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0)	break;

		// ���� �����͸� ������ ���
		buf[retval] = '\0';
		printf("[TCP/%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), buf);

		// ���� �����͸� �ش� Ŭ���̾�Ʈ���� ����
		for (int i = 0; i < num; i++) {
			//printf("%d", i);
			retval = send(sockets[i], buf, retval, 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
		}
	}

	// ���� �ݱ�
	closesocket(client_sock); num--;
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d, ���� ���=%d\n", addr, ntohs(clientaddr.sin_port),num);
	return 0;
}

int main(int argc, char *argv[])
{
	printf("[���� ���α׷��� �����մϴ�.]\n");
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;
	else printf("  -> [Winsock�� ���������� �ʱ�ȭ�Ǿ����ϴ�.]\n");

	// ���� ����
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");
	else printf("  -> [socket() �Լ��� ȣ��Ǿ����ϴ�.]\n");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");
	else printf("  -> [bind() �Լ��� ȣ��Ǿ����ϴ�.]\n");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");
	else printf("  -> [listen() �Լ��� ȣ��Ǿ����ϴ�.]\n");

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	HANDLE hThread;

	while (1) {
		// accept()
		printf("  -> [accept() �Լ��� ȣ���Ͽ� ���ο� Client�� ��ٸ��ϴ�...]\n");
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
		sockets[num++] = client_sock;	// ���� client_sock�� sockets �迭�� ����
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		else printf("  -> [Client�� ������ �õ��Ͽ� accept()�� �Ϸ�Ǿ����ϴ�.]\n");

		// ������ Ŭ���̾�Ʈ ���� ���
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d, ������ ��=%d\n", addr, ntohs(clientaddr.sin_port), num);

		// ������ ����
		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)client_sock, 0, NULL);
		if (hThread == NULL) { closesocket(client_sock); num--; }
		else { CloseHandle(hThread); }
	}

	// ���� �ݱ�
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}
