#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "valve_sdk/Misc/Color.hpp"

#define A( s ) #s
#define OPTION(type, var, val) Var<type> var = {A(var), val}

template <typename T = bool>
class Var {
public:
	std::string name;
	std::shared_ptr<T> value;
	int32_t size;
	Var(std::string name, T v) : name(name) {
		value = std::make_shared<T>(v);
		size = sizeof(T);
	}
	operator T() { return *value; }
	operator T*() { return &*value; }
	operator T() const { return *value; }
};

class Options
{
public:
		OPTION(bool, esp_player_boxes, false);
		OPTION(bool, esp_player_names, false);
		OPTION(bool, esp_player_health, false);
		OPTION(bool, esp_player_armour, false);
		OPTION(bool, esp_player_weapons, false);
		OPTION(bool, esp_player_ammo, false);

		OPTION(bool, esp_planted_c4, false);
		OPTION(bool, esp_items, true);
		OPTION(bool, esp_dropped_weapons, true);

		OPTION(bool, glow, false);
		OPTION(bool, chams, false);
		OPTION(bool, chams_xyz, false);
		OPTION(bool, misc_remove_scope, false);
		OPTION(bool, misc_bomb_timer, false);
		OPTION(bool, misc_desync, false);
		OPTION(bool, misc_silentwalk, false);
		OPTION(bool, misc_showimpacts, false);
		OPTION(bool, misc_fullbright, false);
		OPTION(bool, misc_triggerbot, false);
		OPTION(int, misc_fakelag_amount, 0);
		OPTION(int, misc_triggerbot_hotkey, 0);
		OPTION(bool, misc_spectators, false);
		OPTION(int, misc_desync_hotkey, 0);
		OPTION(bool, misc_hitmarker, false);
		OPTION(bool, misc_backtrack, false);
		OPTION(float, misc_aspect_ratio, 0);
		OPTION(bool, misc_bhop, false);
		OPTION(bool, misc_jumpbug, false);
		OPTION(int, misc_jumpbug_hotkey, 0);
		OPTION(bool, misc_rcs, false);
		OPTION(bool, misc_info, false);
		OPTION(bool, misc_thirdperson, false);
		OPTION(int, misc_thirdperson_hotkey, 0);
		OPTION(int, viewmodel_fov, 68);
		OPTION(int, viewmodel_offset_x, 1);
		OPTION(int, viewmodel_offset_y, 1);
		OPTION(int, viewmodel_offset_z, -1);
    	OPTION(float, misc_nightmode, 1.f);
		OPTION(bool, enemies_only, false);
		OPTION(Color, color_glow, Color(255, 0, 0));
		OPTION(Color, color_chams_visible, Color(255, 0, 0));
		OPTION(Color, color_chams_occluded, Color(255, 128, 0));
};

inline Options g_Options;
inline bool   g_Unload;
