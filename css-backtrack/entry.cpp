#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <cstdint>
#include "minhook/MinHook.h"
#include "c_usercmd.hpp"
#include <vector>
#include <optional>
#include "vector.hpp"
#include <intrin.h>

#define M_PI 3.14159265359f
#define MD_PI 3.14159265359

void unload(void* instance);

const wchar_t* title = L"debug";
bool attached = false;

struct mat3x4 {
	float c[3][4];
};

static void* g_client = nullptr;
static void* g_client_mode = nullptr;

using interface_fn = void* (*)();
class interface_registration {
public:
	interface_registration() = delete;

	interface_fn            create_fn;
	const char* name;
	interface_registration* next;
};

// get interface funciton
template<typename T>
static T* get_interface(const char* mod, const char* iface, bool exact = false) {
	T* interface_ptr = nullptr;
	interface_registration* register_list;
	int part_match_len = strlen(iface);

	DWORD interface_fn = reinterpret_cast<DWORD>(GetProcAddress(GetModuleHandleA(mod), "CreateInterface"));
	if (!interface_fn) { return nullptr; }

	unsigned int jump_start = (unsigned int)(interface_fn) + 4;
	unsigned int jump_target = jump_start + *(unsigned int*)(jump_start + 1) + 5;

	register_list = **reinterpret_cast<interface_registration***>(jump_target + 6);

	for (interface_registration* cur = register_list; cur; cur = cur->next) {
		if (exact) {
			if (strcmp(cur->name, iface) == 0)
				interface_ptr = reinterpret_cast<T*>(cur->create_fn());
		}
		else {
			if (!strncmp(cur->name, iface, part_match_len) && std::atoi(cur->name + part_match_len) > 0)
				interface_ptr = reinterpret_cast<T*>(cur->create_fn());
		}
	}

	return interface_ptr;
}

namespace backtrack {
	struct record_t {
		vec3_t head;
		vec3_t origin;
		int simulation_time;
	};

	std::vector<record_t> records[65];

	std::optional<record_t> find_last_record(int idx) {
		if (records[idx].empty()) {
			return std::nullopt;
		}

		return std::make_optional(records[idx].back());
	}
}

constexpr auto deg_per_rad = 57.2957795131f;

float rad_to_deg(float x) {
	return x * deg_per_rad;
}

void vector_angles(vec3_t& forward, vec3_t& angles) {
	if (forward.x == 0.f && forward.y == 0.f) {
		angles.x = (forward.z > 0.f) ? 270.f : 90.f;
		angles.y = 0.f;
	}
	else {
		angles.x = -rad_to_deg(atan2(-forward.z, forward.length_2d()));
		angles.y = rad_to_deg(atan2(forward.y, forward.x));

		if (angles.y > 90.f)
			angles.y -= 180.f;
		else if (angles.y < 90.f)
			angles.y += 180.f;
		else if (angles.y == 90.f)
			angles.y = 0.f;
	}

	angles.z = 0.f;
}

union ieee754_float {
	uint32_t full;

	struct
	{
		uint32_t fraction : 23;
		uint32_t exponent : 8;
		uint32_t sign : 1;
	};
};

vec3_t calc_angle(const vec3_t& from, const vec3_t& to) {
	vec3_t angles;
	vec3_t delta = from - to;

	vector_angles(delta, angles);

	angles.clamp();

	return angles;
}

float abs_val(float value) {
	auto copy_val = value;
	reinterpret_cast<ieee754_float*>(&copy_val)->sign = 0;

	return copy_val;
}

float calc_fov(const vec3_t& from, const vec3_t& to, const vec3_t& view_angle) {
	auto ideal_angle = calc_angle(from, to);

	ideal_angle -= view_angle;

	if (ideal_angle.x > 180.f)
		ideal_angle.x -= 360.f;
	else if (ideal_angle.x < -180.f)
		ideal_angle.x += 360.f;

	if (ideal_angle.y > 180.f)
		ideal_angle.y -= 360.f;
	else if (ideal_angle.y < -180.f)
		ideal_angle.y += 360.f;

	ideal_angle.x = abs_val(ideal_angle.x);
	ideal_angle.y = abs_val(ideal_angle.y);

	return ideal_angle.x + ideal_angle.y;
}

// create move hook
using create_move_t = bool(__thiscall*)(void*, float, UserCmd*);
create_move_t o_create_move;

bool __fastcall hk_create_move(void* thisptr, void*, float input_sample_frametime, UserCmd* cmd) {
	if (!cmd || !cmd->command_number)
		return o_create_move(thisptr, input_sample_frametime, cmd);

	static uint32_t client_module = reinterpret_cast<uint32_t>(GetModuleHandleA("client.dll"));

	const auto local_player = *reinterpret_cast<uint32_t*>(client_module + 0x004C88E8);

	if (!local_player)
		return o_create_move(thisptr, input_sample_frametime, cmd);

	/*
	// get flags
	const auto flags = *reinterpret_cast<uint32_t*>(local_player + 0x350);
	if (cmd->buttons & IN_JUMP && !(flags & FL_ONGROUND))
		cmd->buttons &= ~(IN_JUMP);
	*/

	using namespace backtrack;

	int closest = -1;
	float delta = FLT_MAX;

	Vector viewangles = cmd->viewangles;

	// loop through all entities
	for (auto i = 0; i < 64; i++) {
		const auto entity = *reinterpret_cast<uint32_t*>(client_module + 0x004D5AE4 + i * 0x10);

		if (!entity) {
			if (!records[i].empty()) {
				records[i].clear();
			}
			continue;
		}

		if (entity == local_player) {
			if (!records[i].empty()) {
				records[i].clear();
			}
			continue;
		}

		const auto team = *reinterpret_cast<uint32_t*>(entity + 0x9c);

		if (team == *reinterpret_cast<uint32_t*>(local_player + 0x9c)) {
			if (!records[i].empty()) {
				records[i].clear();
			}
			continue;
		}

		const auto health = *reinterpret_cast<uint32_t*>(entity + 0x94);
		if (health <= 0) {
			if (!records[i].empty()) {
				records[i].clear();
			}
			continue;
		}

		const auto life_state = *reinterpret_cast<uint32_t*>(entity + 0x93);
		if (life_state == 257) {
			if (!records[i].empty()) {
				records[i].clear();
			}
			continue;
		}

		record_t record;
		uint32_t bone_matrix = *reinterpret_cast<uint32_t*>(entity + 0x578);
		mat3x4 bone_pos = *reinterpret_cast<mat3x4*>(bone_matrix + 13 * 0x30);
		record.head = vec3_t(bone_pos.c[0][3], bone_pos.c[1][3], bone_pos.c[2][3]);

		record.origin = *reinterpret_cast<vec3_t*>(entity + 0x338);
		record.simulation_time = cmd->tick_count;

		records[i].insert(records[i].begin(), record);

		if (records[i].size() > 24) { // 24 ticks because of maxunlag
			records[i].pop_back();
		}

		vec3_t view_angle = vec3_t(viewangles.x, viewangles.y, viewangles.z);
		
		auto player_head = *reinterpret_cast<vec3_t*>(local_player + 0x338);
		player_head.z += *reinterpret_cast<float*>(local_player + 0xf0);
		
		auto enemy_eyes = *reinterpret_cast<vec3_t*>(entity + 0x338);
		enemy_eyes.z += 72.0f;

		// get fov
		auto fov = calc_fov(player_head, enemy_eyes, view_angle);
		if (fov < delta) {
			delta = fov;
			closest = i;
		}
		
	}

	int index = 0;

	if (closest != -1) {
		for (uint32_t i = 0; i < records[closest].size(); i++) {
			
			auto player_head = *reinterpret_cast<vec3_t*>(local_player + 0x338);
			player_head.z += 72.0f;

			float fov = calc_fov(player_head, records[closest][i].head, vec3_t(viewangles.x, viewangles.y, viewangles.z));

			if (fov < delta) {
				index = i;
				delta = fov;
			}
		}

		if (index && cmd->buttons & IN_ATTACK) {
			cmd->tick_count = records[closest].at(index).simulation_time;
			//printf("tick: %d\n", cmd->tick_count);
		}
	}

	return o_create_move(thisptr, input_sample_frametime, cmd);
}

void main_thread(void* instance) {

	// setup console
	attached = AllocConsole() && SetConsoleTitleW(title);
	freopen_s(reinterpret_cast<FILE**>stdin, "CONIN$", "r", stdin);
	freopen_s(reinterpret_cast<FILE**>stdout, "CONOUT$", "w", stdout);

	if (!attached)
		unload(instance);

	g_client = get_interface<void>("client", "VClient");
	g_client_mode = **(void***)((*(uintptr_t**)g_client)[10] + 0x5); // access the 10th index of the vtable and add 5 bytes to get the client mode pointer

	MH_Initialize();
	MH_CreateHook((*static_cast<void***>(g_client_mode))[21], &hk_create_move, reinterpret_cast<void**>(&o_create_move));
	MH_EnableHook(MH_ALL_HOOKS);

	while (!GetAsyncKeyState(VK_END)) {

		Sleep(200);
	}

	unload(instance);
}

void unload(void* instance) {

	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);
	MH_Uninitialize();

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

	CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)main_thread, instance, 0, nullptr));

	return TRUE;
}