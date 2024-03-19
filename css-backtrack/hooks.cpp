#include "hooks.hpp"
#include "minhook/MinHook.h"
#include "interface.hpp"

void hooks::init() {

	interfaces::g_client = interfaces::get_interface<void>("client", "VClient");
	interfaces::g_client_mode = **(void***)((*(uintptr_t**)interfaces::g_client)[10] + 0x5); // access the 10th index of the vtable and add 5 bytes to get the client mode pointer

	MH_Initialize();
	MH_CreateHook((*static_cast<void***>(interfaces::g_client_mode))[21], &hooks::hk_create_move, reinterpret_cast<void**>(&hooks::o_create_move));
	MH_EnableHook(MH_ALL_HOOKS);
}

void hooks::shutdown() {
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}