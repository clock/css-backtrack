#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <cstdint>

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

	while (!GetAsyncKeyState(VK_END)) {

		
		
	}

	unload(instance);
}


void unload(void* instance) {
	auto console_window = GetConsoleWindow();
	FreeConsole();
	PostMessageA(console_window, WM_QUIT, 0, 0);
	FreeLibraryAndExitThread((HMODULE)instance, 0);
}

int __stdcall DllMain(void* instance, unsigned long reason_to_call, void* reserved) {
	if (reason_to_call != DLL_PROCESS_ATTACH)
		return TRUE;

	CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)main_thread, instance, 0, nullptr));

	return TRUE;
}