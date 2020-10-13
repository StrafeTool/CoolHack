#pragma once

#include "../singleton.hpp"

#include "../render.hpp"
#include "../helpers/math.hpp"
#include "../valve_sdk/csgostructs.hpp"

class Visuals : public Singleton<Visuals>
{
	friend class Singleton<Visuals>;

	CRITICAL_SECTION cs;

	Visuals();
	~Visuals();
public:
	class Player
	{
	public:
		struct
		{
			C_BasePlayer* pl;
			bool          is_enemy;
			bool          is_visible;
			Color         clr;
			Vector        head_pos;
			Vector        feet_pos;
			RECT          bbox;
		} ctx;

		bool Begin(C_BasePlayer * pl);
		void RenderBox(C_BaseEntity* entity);
		void RenderName(C_BaseEntity* entity);
		void RenderWeaponName(C_BaseEntity* entity);
		void RenderAmmo(C_BaseEntity* entity);
		void RenderHealth(C_BaseEntity* entity);
		void RenderArmour();
	};
	void Spectators();
	void Nightmode();
	void RenderWeapon(C_BaseCombatWeapon* ent);
	void RenderPlantedC4(C_BaseEntity* ent);
	void RenderItemEsp(C_BaseEntity* ent);
	void DrawSpeed();
	void DrawKeyPresses();
	void ThirdPerson();
public:
	void AddToDrawList();
	void Render();
	bool ApplyCustomSkin(DWORD hWeapon);
	void SkinChanger();
	void BombTimer(C_BaseEntity* ent);
	struct EconomyItemCfg {
		int nFallbackPaintKit = 0;
		int nFallbackSeed = 0;
		int nFallbackStatTrak = -1;
		int iEntityQuality = 4;
		char* szCustomName = nullptr;
		float flFallbackWear = 0.1f;
	};
	std::unordered_map<int, EconomyItemCfg> g_SkinChangerCfg;
};
