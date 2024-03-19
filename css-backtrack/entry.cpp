#include <windows.h>
#include <iostream>
#include "hooks.hpp"
#include "vector.hpp"

void unload(void* instance);

const wchar_t* title = L"debug";
bool attached = false;

void main_thread(void* instance) {

	// setup console
	attached = AllocConsole() && SetConsoleTitleW(title);
	freopen_s(reinterpret_cast<FILE**>stdin, "CONIN$", "r", stdin);
	freopen_s(reinterpret_cast<FILE**>stdout, "CONOUT$", "w", stdout);

	if (!attached)
		unload(instance);

	hooks::init();

	while (!GetAsyncKeyState(VK_END)) {

		Sleep(200);
	}

	unload(instance);
}

void unload(void* instance) {

	hooks::shutdown();

	Sleep(100);

	auto console_window = GetConsoleWindow();
	FreeConsole();
	PostMessageA(console_window, WM_QUIT, 0, 0);

	Sleep(1000);

	FreeLibraryAndExitThread((HMODULE)instance, 0);
}

int __stdcall DllMain(void* instance, unsigned long reason_to_call, void* reserved) {
	if (reason_to_call != DLL_PROCESS_ATTACH)
		return TRUE;

	HANDLE thread_handle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)main_thread, instance, 0, nullptr);
	if (thread_handle == NULL)
		return 0;

	CloseHandle(thread_handle);

	return 1;
}