#pragma once

namespace interfaces {
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

		unsigned int jump_start = (unsigned int)(interface_fn)+4;
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
}