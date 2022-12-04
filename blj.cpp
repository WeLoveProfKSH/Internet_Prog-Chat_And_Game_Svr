#include "blj.h"

DWORD WINAPI bljsvr(LPVOID lpParam) {
	char c;
	char buf[512];

	while (1)
	{
		c = _getch(); //입력값 input
		if (c == 59) { // F1 키
			_getcwd(buf, 512);				// 현재 실행 경로 얻기
			system(strcat(buf, "\\bljsvr.jar"));
			break;
		}
		Sleep(200);
	}
	return 0;
}