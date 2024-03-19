#pragma once
#include "c_usercmd.hpp"

namespace hooks {
	void init();
	void shutdown();

	using create_move_t = bool(__thiscall*)(void*, float, user_cmd*);
	inline create_move_t o_create_move;

	bool __fastcall hk_create_move(void* thisptr, void*, float input_sample_frametime, user_cmd* cmd);
}