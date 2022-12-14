#include "Common.h"
#include <conio.h>
#include <locale.h>		// Locale 헤더 파일
#include <direct.h>
#include <string>

#define SERVERPORT 9000
#define BUFSIZE    512
#define MAXCLIENTS 20	// 일단 20명

// 접속한 클라이언트 목록
SOCKET sockets[MAXCLIENTS];			// 소켓 담아둘 배열
static int sockets_id[MAXCLIENTS];	// 소켓 배열을 관리하는데 쓸 배열 (id)
static boolean ignore_table[MAXCLIENTS][MAXCLIENTS];	// ignore 저장용
static int num = 0;					// 항상 서버에 접속한 클라이언트 숫자
char send_msg[512];					// send() 함수로 보낼 메시지 저장용
short target_UID = 0;				// /w, /ignore 등 대상 UID 저장용
char cut[20][BUFSIZE + 1]{};		// strtok() 함수로 토막낸 문자열 저장용

int com_preprocessor(int id, char* buf) {	// 잘못된 명령어인지 확인하기
	char* ptr;							// strtok() 용 포인터 변수
	char buf_t[BUFSIZE + 1];			// 임시 저장용
	int tomak_count = 0;				// 받은 문자열 토막낸 횟수 저장하는 변수
	char temp_ch[100];

	strcpy(buf_t, buf);					// strtok() 함수 사용 전 buf의 내용을 buf_t에 복사
	ptr = strtok(buf_t, " ");			// buf_t의 내용을 공백 단위로 토큰화

	while (ptr) {
		strcpy(cut[tomak_count++], ptr);
		ptr = strtok(NULL, " ");
	}
	target_UID = atoi(cut[3]);			// 획득한 {UID}를 atoi() 함수로 문자열에서 int형 정수로 변환하고 target_UID에 저장
	_itoa(target_UID, temp_ch, 10);		// atoi() 함수를 거친 결과를 temp_ch에 문자열로 저장

	if (!(strlen(cut[3]) == strlen(temp_ch))) {
		// 변환 전 {UID} 값과(cut[3]) atoi() 함수로 변환 후의 {UID}값(temp_ch)에 차이가 있는지 확인
		// 만일 둘이 같지 않다면 {UID} 위치에 정수 이외의 값이 섞여있었고 atoi() 함수를 거치며 손실되었다는 의미이므로 오류 표시.
		sprintf(send_msg, "형식 오류 : {UID} 필드에 올바른 범위안의 정수값을 입력해주십시오.");
		send(sockets[id], send_msg, sizeof(send_msg), 0);
		return 0;
	}

	if (!((target_UID >= 0) && (target_UID <= MAXCLIENTS - 1))) {	// 범위 밖의 {UID}가 지정된 경우
		sprintf(send_msg, "범위 오류 : 명령어 다음에 0 ~ %d 범위에 있는 대상 Host의 ID값을 입력하십시오.", MAXCLIENTS - 1);
		send(sockets[id], send_msg, sizeof(send_msg), 0);
		return 0;
	}

	if (sockets_id[target_UID] == -1) {	// 잘못된 {UID}를 지정한 경우 (서버에 연결되지 않은 Host)
		sprintf(send_msg, "해당 UID를 가진 Host가 없습니다.");
		send(sockets[id], send_msg, sizeof(send_msg), 0);
		return 0;
	}
	return tomak_count;
}
int ignore(int id, char* buf) {
	if (com_preprocessor(id, buf)) {
		ignore_table[id][atoi(cut[3])] = true;
		return 1;
	}
	else return 0;
}
int unignore(int id, char* buf) {
	if (com_preprocessor(id, buf)) {
		ignore_table[id][atoi(cut[3])] = false;
		return 1;
	}
	else return 0;
}
int whisper(char *addr, u_short port, int id, char *buf) {		// 귓말 기능
	u_short msg_length = com_preprocessor(id, buf);

	if (msg_length == 0) return 0;
	if (ignore_table[target_UID][id] == true) {	// 수신자가 송신자를 차단해놓은 경우
		return 1;	// 아래 내용들 다 제끼고 그냥 return 1 반환
	}

	sprintf(send_msg, "[ TCP/%hs:%d, UID:%hd ] (#귓말) ", addr, port, id);

	for (int i = 0; i < msg_length; i++) {	// 잘린 문자열들 이어 붙이기
		if (i == 2) continue;		// '/w' 제외
		if (i == 3) continue;		// '{UID} 제외
		strcat(cut[i], " ");		// 이어 붙일 문자열에 공백 하나씩 추가
		strcat(send_msg, cut[i]);	
	}
	wprintf(L"%hs → [%hs]\n", send_msg, cut[3]);				// 서버 창에 누가 누구한테 귓을 보냈는지 띄우기
	send(sockets[target_UID], send_msg, sizeof(send_msg), 0);	// 대상한테(target_UID) 메시지 보내기
	return 1;
}

void sockets_print() {		// sockets_id 상태 확인 (디버깅용)
	if (num == 0) wprintf(L"\n아무도 없습니다.\n");
	else {
		wprintf(L"\n----------------------------- sockets 소켓 배열 상황 -----------------------------\n");
		for (int i = 0; i < MAXCLIENTS; i++) {	// sockets 배열 확인 및 체크
			if (i < 10)	printf("\tsockets[%d] : %d\t\t\t|\t", i, sockets_id[i]);
			else		printf("\tsockets[%d] : %d\t\t|\t", i, sockets_id[i]);

			if (sockets_id[i] == -1) printf("**** \"No connected\" ****\t\n");
			else printf("Alive\n");
		}wprintf(L"----------------------------------------------------------------------------------\n\n");
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
DWORD WINAPI ProcessClient(LPVOID arg){
	int retval;
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;		// 이 클라이언트의 연결 정보
	char addr[INET_ADDRSTRLEN];			// 이 클라이언트의 IP주소 저장
	int addrlen;

	char buf[BUFSIZE + 1];				// recv() 함수로 받은 메시지 저장용
	int id{};
	num++;

	for (int i = 0; i < num + 1; i++) {
		if (sockets_id[i] == -1) {	// 비어 있는 공간이 있다면
			id = i;
			sockets_id[i] = i;
			break;
		}
	}

	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	while (1) {
		retval = recv(client_sock, buf, BUFSIZE, 0);	// 데이터 받기

		if (retval == SOCKET_ERROR || retval == 0) {	//	클라이언트랑 연결 끊기면
			sockets_id[id] = -1;			// 연결 끊기면 id에 -1
			retval = NULL;					// 해당 sockets 내용 초기화

			memset(ignore_table[id], false, sizeof(boolean) * MAXCLIENTS);
			for (int i = 0; i < MAXCLIENTS; i++) {
				ignore_table[i][id] = false;	// 다른 사용자들이 이 사용자 차단해놓은거 해제
			}

			wprintf(L"\n[TCP 서버] 클라이언트 종료: IP 주소=%hs, 포트 번호=%d, 남은 사람=%d\n", addr, ntohs(clientaddr.sin_port), --num);
			closesocket(client_sock);	// 소켓 닫기
			break;
		}

		buf[retval] = '\0';
		if (strstr(buf, " /w ") || strstr(buf, " /m ") || strstr(buf, " /msg ") || strstr(buf, " /whisper ")) {	// 클라이언트가 귓속말 기능을 사용하는 경우
			if(whisper(addr, ntohs(clientaddr.sin_port), id, buf))
				send(sockets[id], "귓말을 보냈습니다.\n", sizeof("귓말을 보냈습니다.\n"), 0);	// 송신자에게 "귓말을 보냈습니다." 메시지 보내기
			else {
				send(sockets[id], "USAGE : </w | /m | /msg | /whisper> {UID} [MSG]\n", sizeof("USAGE : </w | /m | /msg | /whisper> {UID} [MSG]\n"), 0);
			}
			continue;
		}
		if (strstr(buf, " /ignore ") || strstr(buf, " /squelch ")) {	// 클라이언트가 사용자 차단 기능을 사용하는 경우
			if (ignore(id, buf))
				send(sockets[id], "해당 사용자의 말을 무시합니다.\n", sizeof("해당 사용자의 말을 무시합니다.\n"), 0);	// 송신자에게 "귓말을 보냈습니다." 메시지 보내기
			else {
				send(sockets[id], "USAGE : </ignore | /squelch> {UID}\n", sizeof("USAGE : </ignore | /squelch> {UID}\n"), 0);
			}
			continue;
		}
		if (strstr(buf, " /unignore ") || strstr(buf, " /unsquelch ")) {	// 클라이언트가 사용자 차단 해제 기능을 사용하는 경우
			if (unignore(id, buf))
				send(sockets[id], "해당 사용자를 차단해제했습니다.\n", sizeof("해당 사용자를 차단해제했습니다.\n"), 0);	// 송신자에게 "귓말을 보냈습니다." 메시지 보내기
			else {
				send(sockets[id], "USAGE : </unignore | /unsquelch> {UID}\n", sizeof("USAGE : </unignore | /unsquelch> {UID}\n"), 0);
			}
			continue;
		}

		wprintf(L"[ TCP/%hs:%d, sockets[%d] ] ", addr, ntohs(clientaddr.sin_port), id);
		wprintf(L"%hs\n", buf);

		sprintf(send_msg, "[ TCP/%hs:%d, UID:%d ] ", addr, ntohs(clientaddr.sin_port), id);
		strcat(send_msg, buf);

		// 받은 데이터를 클라이언트에게 전송
		for (int i = 0; i < MAXCLIENTS; i++) {
			if (sockets_id[i] == -1) continue;	// 소켓이 중간에 비어 있으면 넘기기
			if (i == id) continue;				// 보낸 사람은 넘기기
			if (ignore_table[i][id] == true)	continue;	// 송신자를 차단해놓은 상태면 그쪽으로는 메시지 전송 X
			send(sockets[i], send_msg, sizeof(send_msg), 0);
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
	bThread = CreateThread(NULL, 0, F_key, cntOfThread, 0, NULL);	// Function키 입력으로 기능 실행하는 스레드

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
		wprintf(L"\n[TCP Svr] 클라이언트 접속: IP 주소=%hs, 포트 번호=%d, 접속자 수=%d\n", addr, ntohs(clientaddr.sin_port), num + 1);

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