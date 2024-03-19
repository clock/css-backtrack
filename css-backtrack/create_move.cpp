#define TICK_RATE 100.f
#define BT_TIME 0.125f

#include "hooks.hpp"
#include "vector.hpp"
#include <vector>
#include <optional>
#include "math.hpp"
#include <map>

static uint32_t engine_module;
static std::map<int, std::vector<vec3_t>> player_records = {};

struct temp {
	vec3_t head;
	float simulation_time;
};

static temp target;

bool __fastcall hooks::hk_create_move(void* thisptr, void*, float input_sample_frametime, user_cmd* cmd) {
	if (!cmd || !cmd->command_number)
		return o_create_move(thisptr, input_sample_frametime, cmd);

	static uint32_t client_module = reinterpret_cast<uint32_t>(GetModuleHandleA("client.dll"));
	engine_module = reinterpret_cast<uint32_t>(GetModuleHandleA("engine.dll"));

	const auto local_player = *reinterpret_cast<uint32_t*>(client_module + 0x004C88E8);

	if (!local_player) {
		player_records.clear();
		return o_create_move(thisptr, input_sample_frametime, cmd);
	}

	Vector viewangles = cmd->viewangles;

	// check if local player is null
	if (!local_player) {
		player_records.clear();
		return o_create_move(thisptr, input_sample_frametime, cmd);
	}
	// check if local player is alive
	const auto local_player_health = *reinterpret_cast<uint32_t*>(local_player + 0x94);
	if (local_player_health <= 0) {
		player_records.clear();
		return o_create_move(thisptr, input_sample_frametime, cmd);
	}

	// check life state
	const auto local_player_life_state = *reinterpret_cast<uint32_t*>(local_player + 0x93);
	if (local_player_life_state == 257) {
		player_records.clear();
		return o_create_move(thisptr, input_sample_frametime, cmd);
	}

	std::vector<vec3_t> this_tick = {};

	// loop through all entities
	for (auto i = 0; i < 64; i++) {
		const auto entity = *reinterpret_cast<uint32_t*>(client_module + 0x004D5AE4 + i * 0x10);

		if (!entity)
			continue;

		if (entity == local_player)
			continue;

		const auto team = *reinterpret_cast<uint32_t*>(entity + 0x9c);

		if (team == *reinterpret_cast<uint32_t*>(local_player + 0x9c))
			continue;

		// check health
		const auto health = *reinterpret_cast<uint32_t*>(entity + 0x94);
		if (health <= 0)
			continue;

		const auto life_state = *reinterpret_cast<uint32_t*>(entity + 0x93);
		if (life_state == 257)
			continue;

		// check if dormant
		const auto dormant = *reinterpret_cast<bool*>(entity + 0x17E);

		if (dormant)
			continue;

		// get head position
		uint32_t bone_matrix = *reinterpret_cast<uint32_t*>(entity + 0x578);
		mat3x4 bone_pos = *reinterpret_cast<mat3x4*>(bone_matrix + 14 * 0x30);
		vec3_t head = vec3_t(bone_pos.c[0][3], bone_pos.c[1][3], bone_pos.c[2][3]);

		this_tick.push_back(head);
	}

	player_records[cmd->tick_count] = this_tick;

	// purge old player position data
	for (auto i = player_records.begin(); i != player_records.end();) {
		const int fromTick = i->first;
		if ((cmd->tick_count - fromTick) * (1.f / TICK_RATE) > BT_TIME)
			player_records.erase(i++->first);
		else
			++i;
	}

	// purge old target data if not attacking or simulation time is old
	if (((cmd->buttons & IN_ATTACK) == 0) || (cmd->tick_count - target.simulation_time) * (1.f / TICK_RATE) > BT_TIME) {
		target.head = vec3_t(0, 0, 0);
	}

	auto punch_angle = *reinterpret_cast<vec3_t*>(local_player + (0x0DDC + 0x6C));

	// iterate through old player position data if firing
	if (cmd->buttons & IN_ATTACK) {
		bool found_hittable = false;
		for (auto i = player_records.begin(); i != player_records.end(); i++) {
			const int recorded_tick = i->first;
			for (size_t record = 0; record < i->second.size(); record++) {
				auto eye_pos = *reinterpret_cast<vec3_t*>(local_player + 0x338);
				eye_pos.z += *reinterpret_cast<float*>(local_player + 0xf0);
				// get punch angle
				vec3_t goal = calc_angle(eye_pos, i->second[record] - punch_angle * 2.0f);
				if (calc_fov(eye_pos, i->second[record], vec3_t(viewangles.x, viewangles.y, viewangles.z) + (punch_angle * 2.0f)) < 2.f) {
					cmd->tick_count = recorded_tick;

					found_hittable = true;
					break;
				}
			}
			if (found_hittable)
				break;
		}
	}

	return o_create_move(thisptr, input_sample_frametime, cmd);
}