#include "Common.h"
#include <conio.h>
#include <locale.h>				// Locale 헤더 파일
#include <direct.h>

#define SERVERPORT 9000
#define BUFSIZE    512
#define MAXCLIENTS 20			// 일단 20명

// 접속한 클라이언트 목록
SOCKET sockets[MAXCLIENTS];		// 소켓 담아둘 배열
static int sockets_id[MAXCLIENTS];		// 소켓 배열을 관리하는데 쓸 배열 (id)
static int num = 0;					// 항상 서버에 접속한 클라이언트 숫자
char send_msg[512];

void sockets_print() {		// sockets_id 상태 확인 (디버깅용)
	if (num == 0) wprintf(L"\n아무도 없습니다.\n");
	else{
		wprintf(L"\n-------------------- sockets 소켓 배열 상황 --------------------\n");
		for (int i = 0; i < MAXCLIENTS; i++) {	// sockets 배열 확인 및 체크
			printf("\tsockets[%d] : %d\t\t|\t", i, sockets_id[i]);
			if (sockets_id[i] == -1) printf("**** \"No connected\" ****\t\n");
			else printf("Alive\n");
		}wprintf(L"----------------------------------------------------------------\n\n");
	}
}

DWORD WINAPI F_key(LPVOID lpParam) {	// F1 ~ F2키 기능
	char c;
	char buf[512];

	while (1)
	{
		c = _getch();	//입력값 input

		if (c == 59) {		// F1 키
			_getcwd(buf, 512);	// 현재 실행 경로 얻기
			system(strcat(buf, "\\bljsvr.jar"));
			continue;
		}
		else if (c == 60) {	//	F2
			sockets_print();	
		}
		Sleep(200);
	}
	return 0;
}

// 클라이언트와 데이터 통신
DWORD WINAPI ProcessClient(LPVOID arg)
{
	num++;	
	int id;
	for (int i = 0; i < num + 1; i++) {
		if (sockets_id[i] == -1) {	// 비어 있는 공간이 있다면
			id = i;
			sockets_id[i] = i;
			break;
		}
	}

	int retval;
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	char buf[BUFSIZE + 1];

	//sockets_print();
	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));


	while (1) {
		// 데이터 받기
		retval = recv(client_sock, buf, BUFSIZE, 0);

		if (retval == SOCKET_ERROR || retval == 0) {	//	클라이언트랑 연결 끊기면
			sockets_id[id] = -1;			// 연결 끊기면 id에 -1
			retval = NULL;					// 해당 sockets 내용 초기화
			//sockets_print();

			//wprintf(L"해당 host id :%d", id);	// 디버그용 출력 (무시)
			wprintf(L"\n[TCP 서버] 클라이언트 종료: IP 주소=%hs, 포트 번호=%d, 남은 사람=%d\n", addr, ntohs(clientaddr.sin_port), num-1);

			num--;
			closesocket(client_sock);	// 소켓 닫기
			break;

		}else {		// 클라이언트가 끊기지 않았다면 받은 데이터를 서버에 출력
			buf[retval] = '\0';
			wprintf(L"[ TCP/%hs:%d, sockets[%d] ] ", addr, ntohs(clientaddr.sin_port), id);
			sprintf(send_msg, "[ TCP/%hs:%d, UID:%d ] ", addr, ntohs(clientaddr.sin_port), id);

			wprintf(L"%hs\n", buf);
			strcat(send_msg, buf);

			// 받은 데이터를 클라이언트에게 전송
			for (int i = 0; i < MAXCLIENTS; i++) {
				if (sockets_id[i] == -1) continue;	// 소켓이 중간에 비어 있으면 넘기기
				if (i == id) continue;				// 보낸 사람은 넘기기

				send(sockets[i], send_msg, sizeof(send_msg), 0);
			}
		}
	}
	return 0;
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "korean");		// Locale 설정
	wprintf(L"F1 : 블랙잭 서버 실행 / F2 : 현재 sockets 배열 상태 확인\n\n");
	wprintf(L"[서버 프로그램을 시작합니다.]\n");
	memset(sockets_id, -1, sizeof(int) * MAXCLIENTS);	// sockets_id -1로 초기화
	int retval;
	int index = 0;

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
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
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
	HANDLE bThread = NULL;

	LPVOID cntOfThread = 0;
	bThread = CreateThread(NULL, 0, F_key, cntOfThread, 0, NULL);	// F1키 누르면 블랙잭 서버 실행

	while (1) {
		// accept()
		wprintf(L"  -> [accept() 함수를 호출하여 새로운 Client를 기다립니다...]\n");
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);


		for (int i = 0; i < MAXCLIENTS; i++) {
			if (sockets_id[i] == -1) {	// 비어 있는 공간이 있다면
				sockets[i] = client_sock;
				//sockets_id[i] = i;
				break;
			}
		}		
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
		if (hThread == NULL) {
			CloseHandle(hThread);
			break;
		}
	}

	// 소켓 닫기
	CloseHandle(bThread);
	CloseHandle(hThread);
	closesocket(client_sock);
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}