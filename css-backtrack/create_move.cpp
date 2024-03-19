#include "hooks.hpp"
#include "vector.hpp"
#include <vector>
#include <optional>
#include "math.hpp"

struct mat3x4 {
	float c[3][4];
};

namespace backtrack {
	struct record_t {
		vec3_t head;
		vec3_t origin;
		int simulation_time;
	};

	std::vector<record_t> records[65];

	static record_t selected_record;

	std::optional<record_t> find_last_record(int idx) {
		if (records[idx].empty()) {
			return std::nullopt;
		}

		return std::make_optional(records[idx].back());
	}
}


bool __fastcall hooks::hk_create_move(void* thisptr, void*, float input_sample_frametime, user_cmd* cmd) {
	if (!cmd || !cmd->command_number)
		return o_create_move(thisptr, input_sample_frametime, cmd);

	static uint32_t client_module = reinterpret_cast<uint32_t>(GetModuleHandleA("client.dll"));

	const auto local_player = *reinterpret_cast<uint32_t*>(client_module + 0x004C88E8);

	if (!local_player)
		return o_create_move(thisptr, input_sample_frametime, cmd);

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

		if (index) {
			selected_record = records[closest].at(index);
			if (cmd->buttons & IN_ATTACK) {
				cmd->tick_count = records[closest].at(index).simulation_time;
				//printf("tick: %d\n", cmd->tick_count);
			}
		}
	}

	return o_create_move(thisptr, input_sample_frametime, cmd);
}