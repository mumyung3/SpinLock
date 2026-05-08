#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

bool Flag[2] = {};
int turn = 2;
int g_counter = 0;
int non_critical_count1 = 0;
volatile LONG g_Lock = 0;
bool g_alreadyexit = false;
volatile LONG g_SpinLock = 0;


void Lock() {
	while (InterlockedExchange(&g_SpinLock, 1) == 1) {
		// 1을 썼는데 반환값이 1이면 이미 잠김 → 재시도
	}
}

// 락 해제
void Unlock() {
	InterlockedExchange(&g_SpinLock, 0);
}


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

	//Sleep(10000);

	return 0;
}

unsigned int __stdcall Thread0(void* arg) {

	for (int i = 0; i < 100000; i++) {
		Flag[0] = true;
		turn = 0;

		while (1) {
			if (Flag[1] == false) {
				break;
			}
			if (turn != 0) {

				break;
			}
		}



		if (InterlockedIncrement(&g_Lock) != 1) __debugbreak();  // 즉시 체크
		g_counter++;
		if (InterlockedDecrement(&g_Lock) != 0) __debugbreak();  // 나가기 전 체크

		//if (InterlockedExchange(&g_Lock, 1) == 1) {
		//	__debugbreak();  // 이미 1이었음 = 누군가 있음
		//}

		//g_counter++;

		//// 퇴장 체크
		//if (InterlockedExchange(&g_Lock, 0) != 1) {
		//	__debugbreak();  // 1이 아니었음 = 이상한 상태
		//}

		Flag[0] = false;
	}
	return 0;
}
unsigned int __stdcall Thread1(void* arg) {

	//이전 변수 값 저장
	for (int i = 0; i < 100000; i++) {

		Flag[1] = true;
		turn = 1;
		while (1) {
			if (Flag[0] == false) {
				break;
			}
			if (turn != 1) {
				break;
			}
		}


		if (InterlockedIncrement(&g_Lock) != 1) __debugbreak();  // 즉시 체크
		g_counter++;
		if (InterlockedDecrement(&g_Lock) != 0) __debugbreak();  // 나가기 전 체크
		
		//if (InterlockedExchange(&g_Lock, 1) == 1) {
		//	__debugbreak();  // 이미 1이었음 = 누군가 있음
		//}

		//g_counter++;

		//// 퇴장 체크
		//if (InterlockedExchange(&g_Lock, 0) != 1) {
		//	__debugbreak();  // 1이 아니었음 = 이상한 상태
		//}




		Flag[1] = false;

	}
	return 0;
}

// 왜 임계구간이 제대로 작동안할까? 
// 추측 (각 스레드마다 레지스터 값을 독립적으로 갖고 있고, 메모리는 공유한다. 메모리 값을 읽기 쓰기 시에, 스레드는 자기 자신의 레지스터 값을 본다. 이때, 다른 스레드에서 메모리 쓰기를 하더라도 이미 로드된 레지스터 반영에 안된다. 그래서 이런 동기화 문제가 나타날 것이라고 추측했다.)

// 편의를 위해 스레드 1에만 디버깅을 해보고 있다. 
// 스레드 1은 flag값이 둘다 true 이고 turn 변수값이 0일 때, (스레드 0 이 양보함.) break를 탈출한다.

// 스레드 1은 (flag 둘다 true 시) turn = 0 일때만 돌것이라고 처음에 추측했다. 즉, turn 1 일때 값을 디버깅을 해보자. 
// -> 해본 결과 캡쳐한 사진과 같이 turn 값이 0 이지만, if문을 뚫고 들어왔다. (생각해보니, 이경우는 turn 값이 지연되서 반영됬기 때문인 것 같다..)

// 그렇다면, 어떻게 임계구간이 같이 도는지 정확하게 확인해 볼수 있을까 생각해보았다. 기존의 사용하는 변수로 알아내는건 내 추측에서 비롯된 것인 것 같다.
// 논리적으로 새변수를 만들어서 새로운 플래그를 만들었다. g_alreadyexit 

// 이후 해당 __debugbreak로 들어가면서 백업된 변수를 확인하면서 경우의 수를 확인해 보고 있다.
// 아직까진 어떤 경우에 임계구간이 뚫리는지 정확하게 모르겠다.












// ** 안봐도 되는 주석
// 또는 둘다 flag가 false로 인지하다가 메모리에 true가 되었는지 서로 인지 못하면서 같이 돌것같다. 이때는 turn 상관없이 돌아갈 것.
