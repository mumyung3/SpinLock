#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <intrin.h>

bool Flag[2] = {};
int turn = 2;
int g_counter = 0;
int non_critical_count1 = 0;
volatile LONG g_Lock = 0;
bool g_alreadyexit = false;

unsigned int __stdcall Thread0(void* arg);
unsigned int __stdcall Thread1(void* arg);
int main() {

	_setmode(_fileno(stdout), _O_U16TEXT);

	HANDLE h[2]{};
	h[0] = (HANDLE)_beginthreadex(NULL, 0, Thread0, NULL, 0, NULL);
	h[1] = (HANDLE)_beginthreadex(NULL, 0, Thread1, NULL, 0, NULL);

	WaitForMultipleObjects(2, h, TRUE, INFINITE);

	wprintf(L"Result : %d (예상값 : 200000 )\n", g_counter);
	wprintf(L"non-critical-count라고 추측되는 구간 (아닐수도 있음.) : %d \n", non_critical_count1);

	CloseHandle(h[0]);
	CloseHandle(h[1]);

	return 0;
}

unsigned int __stdcall Thread0(void* arg) {
	bool bu_Flag = {};
	int bu_Turn = 2;
	for (int i = 0; i < 100000; i++) {
		Flag[0] = true;	// store c
		turn = 0;		// store d

		while (1) {

			_mm_mfence();
			bu_Flag = Flag[1];	// store a
			if (bu_Flag == false) { // load a
				break;
			}
			bu_Turn = turn;	// store b
			if (bu_Turn != 0) {	// load b

				break;
			}
		}



		if (InterlockedIncrement(&g_Lock) != 1) __debugbreak();  // 즉시 체크
		g_counter++;
		if (InterlockedDecrement(&g_Lock) != 0) __debugbreak();  // 나가기 전 체크

		Flag[0] = false; 
		bool bu_Flag = {};
		int bu_Turn = 2;
	}
	return 0;
}
unsigned int __stdcall Thread1(void* arg) {
	bool bu_Flag = {};
	int bu_Turn = 2;
	for (int i = 0; i < 100000; i++) {

		Flag[1] = true;
		turn = 1;
		while (1) {
			_mm_mfence(); // store x - load y 바뀔수 있음을 인지하고 코드 짜기 및 그럼에도 문제가 있다? 상관없는 인터락을 쓰든 펜스를 치기. 
			bu_Flag = Flag[0];
			if (bu_Flag == false) {
				break;
			}
			bu_Turn = turn;
			if (bu_Turn != 1) {
				break;
			}
		}


		if (InterlockedIncrement(&g_Lock) != 1) __debugbreak();  // 즉시 체크
		g_counter++;
		if (InterlockedDecrement(&g_Lock) != 0) __debugbreak();  // 나가기 전 체크
		


		Flag[1] = false;
		bool bu_Flag = {};
		int bu_Turn = 2;
	}
	return 0;
}
