#include "Common.h"
#include <conio.h>	// 추후 키보드 입력 감지를 위한 kbhit()함수 사용을 위한 Header
#include <locale.h>	// Locale 헤더 파일

#define SERVERPORT 9000
#define BUFSIZE    512
#define MAXCLIENTS 20		// 일단 20명

// 접속한 클라이언트 목록
SOCKET sockets[MAXCLIENTS];
SOCKET temp;
int test[MAXCLIENTS];
static int num = 0;	// 항상 서버에 접속한 클라이언트 숫자
static int del_num = 0;	// 접속을 끊은 클라이언트 숫자

// 클라이언트와 데이터 통신
DWORD WINAPI ProcessClient(LPVOID arg)
{
	int retval;
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	char buf[BUFSIZE + 1];
	int id = num-1; // ProcessClient() 시작 시 고유 id는 num 찍어주기

	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr *)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));


	while (1) {
		// 데이터 받기
		retval = recv(client_sock, buf, BUFSIZE, 0);
		//printf("%d", retval);
		if (retval == SOCKET_ERROR || retval == 0){	//	클라이언트랑 연결 끊기면
			retval = NULL;	// 해당 sockets 내용 초기화
			num--;
			del_num++;

			//wprintf(L"해당 host id :%d", id);	// 디버그용 출력 (무시)
			wprintf(L"[TCP 서버] 클라이언트 종료: IP 주소=%hs, 포트 번호=%d, 남은 사람=%d\n", addr, ntohs(clientaddr.sin_port), num);

			for (int i = id; i < num; i++) {	// 클라이언트 한 명이랑 연결 끊기면 sockets 배열 위로 하나씩 밀어주기
				wprintf(L"sockets[%d]의 값을 sockets[%d]에 복사했습니다.\n", i+1, i);	//디버그용 출력 (무시)
				sockets[i] = sockets[i + 1];
			}

			//for (int i = 0; i < num; i++) {	// 소팅후 배열 확인용
			//	printf("sockets[%lld] : ", sockets[i]);
			//}

			// 소켓 닫기
			closesocket(client_sock);
			break;
		}else{		// 클라이언트가 끊기지 않았다면
					// 받은 데이터를 서버에 출력
			buf[retval] = '\0';
			wprintf(L"[TCP/%hs:%d] ", addr, ntohs(clientaddr.sin_port));
			printf("%hs\n", buf);

			// 받은 데이터를 해당 클라이언트에게 전송
			for (int i = 0; i < num; i++) {
				retval = send(sockets[i], buf, retval, 0);
				if (retval == SOCKET_ERROR) {
					continue;
				}
			}
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "korean");		// Locale 설정
	wprintf(L"[서버 프로그램을 시작합니다.]\n");
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;
	else wprintf(L"  -> [Winsock이 성공적으로 초기화되었습니다.]\n");

	// 소켓 생성
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");
	else wprintf(L"  -> [socket() 함수가 호출되었습니다.]\n");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");
	else wprintf(L"  -> [bind() 함수가 호출되었습니다.]\n");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");
	else wprintf(L"  -> [listen() 함수가 호출되었습니다.]\n");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	HANDLE hThread = NULL;

	while (1) {
		// accept()
		wprintf(L"  -> [accept() 함수를 호출하여 새로운 Client를 기다립니다...]\n");
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
		sockets[num] = client_sock;	// 현재 client_sock을 sockets 배열에 저장
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		else wprintf(L"  -> [Client가 접속을 시도하여 accept()가 완료되었습니다.]\n");

		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		wprintf(L"\n[TCP Svr] 클라이언트 접속: IP 주소=%hs, 포트 번호=%d, 접속자 수=%d\n", addr, ntohs(clientaddr.sin_port), num+1);

		// 스레드 생성
		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)client_sock, 0, NULL);
		num++;
	}

	// 소켓 닫기
	CloseHandle(hThread);
	closesocket(client_sock);
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}
