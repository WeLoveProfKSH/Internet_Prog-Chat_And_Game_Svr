#include "blj.h"

DWORD WINAPI bljsvr(LPVOID lpParam) {
	char c;
	char buf[512];

	while (1)
	{
		c = _getch(); //�Է°� input
		if (c == 59) { // F1 Ű
			_getcwd(buf, 512);				// ���� ���� ��� ���
			system(strcat(buf, "\\bljsvr.jar"));
			break;
		}
		Sleep(200);
	}
	return 0;
}