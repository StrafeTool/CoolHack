#include <algorithm>

#include "visuals.hpp"

#include "../options.hpp"
#include "../helpers/math.hpp"
#include "../helpers/utils.hpp"


RECT GetBBox(C_BaseEntity* ent)
{
	RECT rect{};
	auto collideable = ent->GetCollideable();

	if (!collideable)
		return rect;

	auto min = collideable->OBBMins();
	auto max = collideable->OBBMaxs();

	const matrix3x4_t& trans = ent->m_rgflCoordinateFrame();

	Vector points[] = {
		Vector(min.x, min.y, min.z),
		Vector(min.x, max.y, min.z),
		Vector(max.x, max.y, min.z),
		Vector(max.x, min.y, min.z),
		Vector(max.x, max.y, max.z),
		Vector(min.x, max.y, max.z),
		Vector(min.x, min.y, max.z),
		Vector(max.x, min.y, max.z)
	};

	Vector pointsTransformed[8];
	for (int i = 0; i < 8; i++) {
		Math::VectorTransform(points[i], trans, pointsTransformed[i]);
	}

	Vector screen_points[8] = {};

	for (int i = 0; i < 8; i++) {
		if (!Math::WorldToScreen(pointsTransformed[i], screen_points[i]))
			return rect;
	}

	auto left = screen_points[0].x;
	auto top = screen_points[0].y;
	auto right = screen_points[0].x;
	auto bottom = screen_points[0].y;

	for (int i = 1; i < 8; i++) {
		if (left > screen_points[i].x)
			left = screen_points[i].x;
		if (top < screen_points[i].y)
			top = screen_points[i].y;
		if (right < screen_points[i].x)
			right = screen_points[i].x;
		if (bottom > screen_points[i].y)
			bottom = screen_points[i].y;
	}
	return RECT{ (long)left, (long)top, (long)right, (long)bottom };
}

Visuals::Visuals()
{
	InitializeCriticalSection(&cs);
}

Visuals::~Visuals() {
	DeleteCriticalSection(&cs);
}

//--------------------------------------------------------------------------------
void Visuals::Render() {
}
//--------------------------------------------------------------------------------
bool Visuals::Player::Begin(C_BasePlayer* pl)
{
	if (!pl->IsAlive())
		return false;

	ctx.pl = pl;
	ctx.is_enemy = g_LocalPlayer->m_iTeamNum() != pl->m_iTeamNum();
	ctx.is_visible = g_LocalPlayer->CanSeePlayer(pl, HITBOX_CHEST);

	if (!ctx.is_enemy && g_Options.enemies_only)
		return false;

	ctx.clr = Color(255, 255, 255);

	auto head = pl->GetHitboxPos(HITBOX_HEAD);
	auto origin = pl->m_vecOrigin();

	head.z += 15;

	if (!Math::WorldToScreen(head, ctx.head_pos) ||
		!Math::WorldToScreen(origin, ctx.feet_pos))
		return false;

	auto h = fabs(ctx.head_pos.y - ctx.feet_pos.y);
	auto w = h / 1.65f;

	ctx.bbox.left = static_cast<long>(ctx.feet_pos.x - w * 0.5f);
	ctx.bbox.right = static_cast<long>(ctx.bbox.left + w);
	ctx.bbox.bottom = static_cast<long>(ctx.feet_pos.y);
	ctx.bbox.top = static_cast<long>(ctx.head_pos.y);

	return true;
}
//--------------------------------------------------------------------------------
void Visuals::Player::RenderBox(C_BaseEntity* entity) {
	static int alpha = 255;
	alpha += entity->IsDormant() ? -1 : 25;
	if (alpha < 0)
		alpha = 0;
	if (alpha > 255)
		alpha = 255;

	Render::Get().RenderBoxByType(ctx.bbox.left, ctx.bbox.top, ctx.bbox.right, ctx.bbox.bottom, Color(255, 255, 255, alpha), 1);
}
//--------------------------------------------------------------------------------
void Visuals::Player::RenderName(C_BaseEntity* entity)
{
	static int alpha = 255;
	alpha += entity->IsDormant() ? -1 : 25;
	if (alpha < 0)
		alpha = 0;
	if (alpha > 255)
		alpha = 255;
	player_info_t info = ctx.pl->GetPlayerInfo();
	auto sz = g_pDefaultFont->CalcTextSizeA(10.f, FLT_MAX, 0.0f, info.szName);
	Render::Get().RenderText(info.szName, ctx.feet_pos.x - sz.x / 2, ctx.head_pos.y - sz.y / 2, 10.f,  Color(255, 255, 255, alpha));
}
//--------------------------------------------------------------------------------
void Visuals::Player::RenderAmmo(C_BaseEntity* entity) {
	static int alpha = 255;
	alpha += entity->IsDormant() ? -1 : 25;
	if (alpha < 0)
		alpha = 0;
	if (alpha > 255)
		alpha = 255;
	auto weapon = ctx.pl->m_hActiveWeapon().Get();
	if (!weapon) return;
	if (!weapon->GetCSWeaponData()) return;

	auto clip = weapon->m_iClip1();
	auto clip2 = weapon->m_iPrimaryReserveAmmoCount();

	static float Battery = clip2;

	if (weapon->IsKnife())
		return;

	int iClip = ctx.pl->m_hActiveWeapon().Get()->m_iClip1();
	int iClipMax = ctx.pl->m_hActiveWeapon().Get()->GetCSWeaponData()->iMaxClip1;
	float box_w = (float)fabs(ctx.bbox.right - ctx.bbox.left);
	float width;
	width = (((box_w * iClip) / iClipMax));

	g_VGuiSurface->DrawSetColor(Color(0, 0, 0, alpha));
	g_VGuiSurface->DrawOutlinedRect(ctx.bbox.left - 1, ctx.bbox.bottom + 3, ctx.bbox.right + 1, ctx.bbox.bottom + 7);
	if (!entity->IsDormant()) {
		g_VGuiSurface->DrawSetColor(0, 0, 0, 160);
		g_VGuiSurface->DrawFilledRect(ctx.bbox.left - 1, ctx.bbox.bottom + 3, ctx.bbox.right + 1, ctx.bbox.bottom + 7);
	}
	g_VGuiSurface->DrawSetColor(61, 135, 255, alpha);
	g_VGuiSurface->DrawFilledRect(ctx.bbox.left, ctx.bbox.bottom + 4, ctx.bbox.left + width, ctx.bbox.bottom + 6);

	if (iClip < iClipMax)
		Render::Get().RenderText(std::to_string(iClip), ImVec2(ctx.bbox.left + width + 4, ctx.bbox.bottom), 10.f, Color(255, 255, 255, alpha));
}
//--------------------------------------------------------------------------------
void Visuals::Spectators()
{
	if (g_EngineClient->IsConnected()) {

		int wa, ha;
		g_EngineClient->GetScreenSize(wa, ha);

		int pos_x = 10;
		int pos_y = ha / 2 + 20;

		int loop = 0;

		for (int i = 0; i <= 64; i++) {
			C_BasePlayer* e = (C_BasePlayer*)g_EntityList->GetClientEntity(i);
			player_info_t pinfo;

			if (e && e != g_LocalPlayer && !e->IsDormant()) {
				g_EngineClient->GetPlayerInfo(i, &pinfo);
				auto obs = e->m_hObserverTarget();
				if (!obs) continue;
				C_BasePlayer* spec = (C_BasePlayer*)g_EntityList->GetClientEntityFromHandle(obs);
				if (spec == nullptr) continue;
				player_info_t spec_info;
				g_EngineClient->GetPlayerInfo(i, &spec_info);
				char buf[255]; sprintf_s(buf, "%s", pinfo.szName);

				if (strstr(pinfo.szName, "GOTV"))
					continue;

				if (spec->EntIndex() == g_LocalPlayer->EntIndex()) {
					Render::Get().RenderText(buf, ImVec2(pos_x + 2, (pos_y + (14 * loop)) + 26), 13.f, Color(255, 255, 255));
					//g_VGuiSurface->DrawSetColor(Color(61, 135, 255));
					//g_VGuiSurface->DrawFilledRect(pos_x, pos_y + 20, pos_x + 77, pos_y + (14 * loop) + 44);
					//g_VGuiSurface->DrawSetColor(Color(0, 0, 0));
					//g_VGuiSurface->DrawOutlinedRect(pos_x, pos_y + 20, pos_x + 77, pos_y + (14 * loop) + 44);
					loop++;
				}
			}
		}
		char loop_str[24];
		g_VGuiSurface->DrawSetColor(Color(61, 135, 255));
		g_VGuiSurface->DrawFilledRect(pos_x, pos_y + 20, pos_x + 77, pos_y + 24);
		g_VGuiSurface->DrawSetColor(Color(0, 0, 0));
		g_VGuiSurface->DrawOutlinedRect(pos_x, pos_y + 20, pos_x + 77, pos_y + 24);
		Render::Get().RenderText("Spectators", ImVec2(pos_x, pos_y + 4), 16.f, Color(255, 255, 255));
	}
}
//--------------------------------------------------------------------------------
void Visuals::Player::RenderHealth(C_BaseEntity* entity) {

	static float Battery = 4;
	static int alpha = 255;
	alpha += entity->IsDormant() ? -1 : 25;
	if (alpha < 0)
		alpha = 0;
	if (alpha > 255)
		alpha = 255;

	constexpr float SPEED_FREQ = 255 / 2.f;
	int player_hp = ctx.pl->m_iHealth();
	int player_hp_max = 100;
	int Red = 255 - (player_hp * 2.00);
	int Green = player_hp * 2.00;
	static float prev_player_hp[65];

	if (prev_player_hp[ctx.pl->EntIndex()] > player_hp)
		prev_player_hp[ctx.pl->EntIndex()] -= SPEED_FREQ * g_GlobalVars->frametime;
	else
		prev_player_hp[ctx.pl->EntIndex()] = player_hp;

	float x = ctx.bbox.left - 10, y = ctx.bbox.top, width = fabsf(ctx.bbox.bottom - ctx.bbox.top) - (((fabsf(ctx.bbox.bottom - ctx.bbox.top) * prev_player_hp[ctx.pl->EntIndex()]) / player_hp_max));
	float w = 4, h = fabsf(ctx.bbox.bottom - ctx.bbox.top);
	float height = (ctx.bbox.bottom - ctx.bbox.top) / Battery;

	Color fill = Color(static_cast<int> ((130.0f - player_hp * 1.3f)), static_cast<int> ((player_hp * 2.55f)), 10, alpha);

	g_VGuiSurface->DrawSetColor(Color(0, 0, 0, alpha));
	g_VGuiSurface->DrawOutlinedRect(x + 3, y - 1, x + w + 3, y + h + 1);
	if (!entity->IsDormant()) {
		g_VGuiSurface->DrawSetColor(0, 0, 0, 160);
		g_VGuiSurface->DrawFilledRect(x + 3, y - 1, x + w + 3, y + h + 1);
	}
	g_VGuiSurface->DrawSetColor(Color(Red, Green, 0, alpha));
	g_VGuiSurface->DrawFilledRect(x + 4, y + width, x + w - 1 + 3, y + h);

	for (int i = 0; i < Battery; i++) {
		g_VGuiSurface->DrawSetColor(Color(0, 0, 0, alpha));
		g_VGuiSurface->DrawLine(x + 3, y + i * height - 1, x + w + 2, y + i * height - 1);
	}
	if (player_hp != 100)
		Render::Get().RenderText(std::to_string(player_hp), ImVec2(x - 4, y + width - 5), 10.f, Color(255, 255, 255, alpha), true);
}
//--------------------------------------------------------------------------------
inline bool Visuals::ApplyCustomSkin(DWORD hWeapon) {
	C_BaseAttributableItem* pWeapon = (C_BaseAttributableItem*)g_EntityList->GetClientEntityFromHandle(hWeapon);

	if (!pWeapon)
		return false;

	int nWeaponIndex = pWeapon->m_Item().m_iItemDefinitionIndex();

	if (g_SkinChangerCfg.find(nWeaponIndex) == g_SkinChangerCfg.end())
		return false;

	pWeapon->m_nFallbackPaintKit() = g_SkinChangerCfg[nWeaponIndex].nFallbackPaintKit;
	pWeapon->m_Item().m_iEntityQuality() = g_SkinChangerCfg[nWeaponIndex].iEntityQuality;
	pWeapon->m_nFallbackSeed() = g_SkinChangerCfg[nWeaponIndex].nFallbackSeed;
	pWeapon->m_nFallbackStatTrak() = g_SkinChangerCfg[nWeaponIndex].nFallbackStatTrak;
	pWeapon->m_flFallbackWear() = g_SkinChangerCfg[nWeaponIndex].flFallbackWear;
	pWeapon->m_Item().m_iItemIDHigh() = -1;

	return true;
}
//--------------------------------------------------------------------------------
void Visuals::SkinChanger() {
	if (!g_LocalPlayer || !g_LocalPlayer->IsAlive())
		return;

	// AWP | Gungnir
	g_SkinChangerCfg[WEAPON_AWP].nFallbackPaintKit = 756;
	g_SkinChangerCfg[WEAPON_AWP].flFallbackWear = 0.00000001f;

	// AK-47 | Case Hardened 
	g_SkinChangerCfg[WEAPON_AK47].nFallbackPaintKit = 44;
	g_SkinChangerCfg[WEAPON_AK47].flFallbackWear = 0.00000001f;
	g_SkinChangerCfg[WEAPON_AK47].nFallbackSeed = 661;

	// M4A4 | Howl
	g_SkinChangerCfg[WEAPON_M4A1].nFallbackPaintKit = 309;
	g_SkinChangerCfg[WEAPON_M4A1].flFallbackWear = 0.00000001f;

	// Glock-18 | Fade
	g_SkinChangerCfg[WEAPON_GLOCK].nFallbackPaintKit = 38;
	g_SkinChangerCfg[WEAPON_GLOCK].flFallbackWear = 0.00000001f;

	// USP-S | Kill Confirmed
	g_SkinChangerCfg[WEAPON_USP_SILENCER].nFallbackPaintKit = 504;
	g_SkinChangerCfg[WEAPON_USP_SILENCER].flFallbackWear = 0.00000001f;

	// Deagle | Blaze
	g_SkinChangerCfg[WEAPON_DEAGLE].nFallbackPaintKit = 37;
	g_SkinChangerCfg[WEAPON_DEAGLE].flFallbackWear = 0.00000001f;

	// SSG08 | Dragonfire
	g_SkinChangerCfg[WEAPON_SSG08].nFallbackPaintKit = 624;
	g_SkinChangerCfg[WEAPON_SSG08].flFallbackWear = 0.00000001f;

	// FAMAS | Commemoration
	g_SkinChangerCfg[WEAPON_FAMAS].nFallbackPaintKit = 919;
	g_SkinChangerCfg[WEAPON_FAMAS].flFallbackWear = 0.00000001f;

	// GALIL | Chatterbox
	g_SkinChangerCfg[WEAPON_GALILAR].nFallbackPaintKit = 398;
	g_SkinChangerCfg[WEAPON_GALILAR].flFallbackWear = 0.00000001f;

	// AUG | Akihabara Accept
	g_SkinChangerCfg[WEAPON_AUG].nFallbackPaintKit = 455;
	g_SkinChangerCfg[WEAPON_AUG].flFallbackWear = 0.00000001f;

	// SG556 | Integrale
	g_SkinChangerCfg[WEAPON_SG556].nFallbackPaintKit = 750;
	g_SkinChangerCfg[WEAPON_SG556].flFallbackWear = 0.00000001f;

	// M4A1-S | Knight
	g_SkinChangerCfg[WEAPON_M4A1_SILENCER].nFallbackPaintKit = 326;
	g_SkinChangerCfg[WEAPON_M4A1_SILENCER].flFallbackWear = 0.00000001f;

	// G3SG1 | The Executioner
	g_SkinChangerCfg[WEAPON_G3SG1].nFallbackPaintKit = 511;
	g_SkinChangerCfg[WEAPON_G3SG1].flFallbackWear = 0.00000001f;

	// SCAR-20 | Bloodsport
	g_SkinChangerCfg[WEAPON_SCAR20].nFallbackPaintKit = 597;
	g_SkinChangerCfg[WEAPON_SCAR20].flFallbackWear = 0.00000001f;

	// Get handles to weapons we're carrying.
	UINT* hWeapons = (UINT*)g_LocalPlayer->m_hMyWeapons();

	if (!hWeapons)
		return;

	// Loop through weapons and run our skin function on them.
	for (int nIndex = 0; hWeapons[nIndex]; nIndex++) {
		ApplyCustomSkin(hWeapons[nIndex]);
	}
}
//--------------------------------------------------------------------------------
void Visuals::BombTimer(C_BaseEntity* ent) {

	if (ent->GetClientClass()->m_ClassID == ClassId::ClassId_CPlantedC4) {
		int x, y; 
		g_EngineClient->GetScreenSize(x, y);
		float flblow = ent->m_flC4Blow();
		float ExplodeTimeRemaining = flblow - (g_LocalPlayer->m_nTickBase() * g_GlobalVars->interval_per_tick);
		float fldefuse = ent->m_flDefuseCountDown();
		float DefuseTimeRemaining = fldefuse - (g_LocalPlayer->m_nTickBase() * g_GlobalVars->interval_per_tick);
		char TimeToExplode[64]; sprintf_s(TimeToExplode, "Explode in: %.1f", ExplodeTimeRemaining);
		char TimeToDefuse[64]; sprintf_s(TimeToDefuse, "Defuse in: %.1f", DefuseTimeRemaining);
		if (ExplodeTimeRemaining > 0 && !ent->m_bBombDefused()) {
			float fraction = ExplodeTimeRemaining / ent->m_flTimerLength();
			int onscreenwidth = fraction * (x / 3);

			// Background 
			g_VGuiSurface->DrawSetColor(178, 34, 34, 255);
			g_VGuiSurface->DrawFilledRect(x / 3, y / 6, x / 3 * 2, y / 6 + 6);
			// Filled Color 
			g_VGuiSurface->DrawSetColor(47, 154, 26, 255);
			g_VGuiSurface->DrawFilledRect(x / 3, y / 6, x / 3 + onscreenwidth, y / 6 + 6);
			// Outline
			g_VGuiSurface->DrawSetColor(0, 0, 0, 255);
			g_VGuiSurface->DrawOutlinedRect(x / 3, y / 6, x / 3 * 2, y / 6 + 6);
			Render::Get().RenderText(TimeToExplode, ImVec2(x / 3 * 1.5, y / 6 - 12), 12, Color(255, 255, 255), true);			
		}
		C_BasePlayer* Defuser = (C_BasePlayer*)C_BasePlayer::get_entity_from_handle(ent->m_hBombDefuser());
		if (Defuser) {
			float fraction = DefuseTimeRemaining / ent->m_flTimerLength();
			int onscreenwidth = fraction * (x / 3);

			// Background 
			g_VGuiSurface->DrawSetColor(30, 30, 30, 255);
			g_VGuiSurface->DrawFilledRect(x / 3, y / 6 + 20, x / 3 * 2, y / 6 + 26);
			// Filled Color 
			g_VGuiSurface->DrawSetColor(100, 255, 255, 255);
			g_VGuiSurface->DrawFilledRect(x / 3, y / 6 + 20, x / 3 + onscreenwidth, y / 6 + 26);
			// Outline
			g_VGuiSurface->DrawSetColor(0, 0, 0, 255);
			g_VGuiSurface->DrawOutlinedRect(x / 3, y / 6 + 20, x / 3 * 2, y / 6 + 26);
			Render::Get().RenderText(TimeToDefuse, ImVec2(x / 3 * 1.5, y / 6 + 8), 12, Color(255, 255, 255), true);
		}
	}
}
//--------------------------------------------------------------------------------
void Visuals::Nightmode() {
	if (!g_EngineClient->IsInGame() && !g_EngineClient->IsConnected())
		return;

	static ConVar* sv_skyname = g_CVar->FindVar("sv_skyname");
	static ConVar* ThreeD = g_CVar->FindVar("r_3dsky");
	static ConVar* r_DrawSpecificStaticProp = g_CVar->FindVar("r_DrawSpecificStaticProp");
	r_DrawSpecificStaticProp->SetValue(1);
	ThreeD->SetValue(0);
	sv_skyname->SetValue("sky_csgo_night02");

	for (MaterialHandle_t i = g_MatSystem->FirstMaterial(); i != g_MatSystem->InvalidMaterial(); i = g_MatSystem->NextMaterial(i))
	{
		IMaterial* pMaterial = g_MatSystem->GetMaterial(i);

		if (!pMaterial)
			continue;

		if (!pMaterial->IsPrecached())
			continue;

		if (strstr(pMaterial->GetTextureGroupName(), "World"))
			pMaterial->ColorModulate(g_Options.misc_nightmode, g_Options.misc_nightmode, g_Options.misc_nightmode);
		if (strstr(pMaterial->GetTextureGroupName(), "StaticProp"))
			pMaterial->ColorModulate(g_Options.misc_nightmode * 2, g_Options.misc_nightmode * 2, g_Options.misc_nightmode * 2);
	}
}
//--------------------------------------------------------------------------------
void Visuals::Player::RenderArmour()
{
	auto  armour = ctx.pl->m_ArmorValue();
	float box_h = (float)fabs(ctx.bbox.bottom - ctx.bbox.top);
	float off = 4;

	int height = (((box_h * armour) / 100));

	int x = ctx.bbox.right + off;
	int y = ctx.bbox.top;
	int w = 4;
	int h = box_h;

	Render::Get().RenderBox(x, y, x + w, y + h, Color::Black, 1.f, true);
	Render::Get().RenderBox(x + 1, y + 1, x + w - 1, y + height - 2, Color(0, 50, 255, 255), 1.f, true);
}
//--------------------------------------------------------------------------------
void Visuals::Player::RenderWeaponName(C_BaseEntity* entity)
{
	auto weapon = ctx.pl->m_hActiveWeapon().Get();
	static int alpha = 255;
	alpha += entity->IsDormant() ? -1 : 25;
	if (alpha < 0)
		alpha = 0;
	if (alpha > 255)
		alpha = 255;

	if (!weapon) return;
	if (!weapon->GetCSWeaponData()) return;
	auto text = weapon->GetCSWeaponData()->szWeaponName + 7;
	if (weapon->IsKnife())
		Render::Get().RenderText(text, ctx.feet_pos.x, ctx.feet_pos.y, 10.f, Color(255, 255, 255, alpha), true);
	else 
		Render::Get().RenderText(text, ctx.feet_pos.x, ctx.feet_pos.y + 7, 10.f, Color(255, 255, 255, alpha), true);
}
//--------------------------------------------------------------------------------
void Visuals::RenderWeapon(C_BaseCombatWeapon* ent)
{
	auto clean_item_name = [](const char* name) -> const char* {
		if (name[0] == 'C')
			name++;

		auto start = strstr(name, "Weapon");
		if (start != nullptr)
			name = start + 6;

		return name;
	};

	// We don't want to Render weapons that are being held
	if (ent->m_hOwnerEntity().IsValid())
		return;

	auto bbox = GetBBox(ent);

	if (bbox.right == 0 || bbox.bottom == 0)
		return;

	Render::Get().RenderBox(bbox, Color(0, 0, 0, 0));


	auto name = clean_item_name(ent->GetClientClass()->m_pNetworkName);

	auto sz = g_pDefaultFont->CalcTextSizeA(14.f, FLT_MAX, 0.0f, name);
	int w = bbox.right - bbox.left;


	Render::Get().RenderText(name, ImVec2((bbox.left + w * 0.5f) - sz.x * 0.5f, bbox.bottom + 1), 12.f, Color(255, 255, 255));
}
//--------------------------------------------------------------------------------
void Visuals::RenderPlantedC4(C_BaseEntity* ent)
{
	auto bbox = GetBBox(ent);

	if (bbox.right == 0 || bbox.bottom == 0)
		return;


	Render::Get().RenderBox(bbox, Color(255, 255, 255));


	int bombTimer = std::ceil(ent->m_flC4Blow() - g_GlobalVars->curtime);
	std::string timer = std::to_string(bombTimer);

	auto name = (bombTimer < 0.f) ? "Bomb" : timer;
	auto sz = g_pDefaultFont->CalcTextSizeA(14.f, FLT_MAX, 0.0f, name.c_str());
	int w = bbox.right - bbox.left;

	Render::Get().RenderText(name, ImVec2((bbox.left + w * 0.5f) - sz.x * 0.5f, bbox.bottom + 1), 14.f, Color(255, 255, 255));
}
//--------------------------------------------------------------------------------
void Visuals::RenderItemEsp(C_BaseEntity* ent)
{
	std::string itemstr = "Undefined";
	const model_t * itemModel = ent->GetModel();
	if (!itemModel)
		return;
	studiohdr_t * hdr = g_MdlInfo->GetStudiomodel(itemModel);
	if (!hdr)
		return;
	itemstr = hdr->szName;
	if (ent->GetClientClass()->m_ClassID == ClassId_CBumpMine)
		itemstr = "";
	else if (itemstr.find("case_pistol") != std::string::npos)
		itemstr = "Pistol Case";
	else if (itemstr.find("case_light_weapon") != std::string::npos)
		itemstr = "Light Case";
	else if (itemstr.find("case_heavy_weapon") != std::string::npos)
		itemstr = "Heavy Case";
	else if (itemstr.find("case_explosive") != std::string::npos)
		itemstr = "Explosive Case";
	else if (itemstr.find("case_tools") != std::string::npos)
		itemstr = "Tools Case";
	else if (itemstr.find("random") != std::string::npos)
		itemstr = "Airdrop";
	else if (itemstr.find("dz_armor_helmet") != std::string::npos)
		itemstr = "Full Armor";
	else if (itemstr.find("dz_helmet") != std::string::npos)
		itemstr = "Helmet";
	else if (itemstr.find("dz_armor") != std::string::npos)
		itemstr = "Armor";
	else if (itemstr.find("upgrade_tablet") != std::string::npos)
		itemstr = "Tablet Upgrade";
	else if (itemstr.find("briefcase") != std::string::npos)
		itemstr = "Briefcase";
	else if (itemstr.find("parachutepack") != std::string::npos)
		itemstr = "Parachute";
	else if (itemstr.find("dufflebag") != std::string::npos)
		itemstr = "Cash Dufflebag";
	else if (itemstr.find("ammobox") != std::string::npos)
		itemstr = "Ammobox";
	else if (itemstr.find("dronegun") != std::string::npos)
		itemstr = "Turrel";
	else if (itemstr.find("exojump") != std::string::npos)
		itemstr = "Exojump";
	else if (itemstr.find("healthshot") != std::string::npos)
		itemstr = "Healthshot";
	else {
		return;
	}
	
	auto bbox = GetBBox(ent);
	if (bbox.right == 0 || bbox.bottom == 0)
		return;
	auto sz = g_pDefaultFont->CalcTextSizeA(14.f, FLT_MAX, 0.0f, itemstr.c_str());
	int w = bbox.right - bbox.left;

	Render::Get().RenderText(itemstr, ImVec2((bbox.left + w * 0.5f) - sz.x * 0.5f, bbox.bottom + 1), 14.f, Color(255, 255, 255));
}
//--------------------------------------------------------------------------------
void Visuals::DrawSpeed() {
	int w, h;
	g_EngineClient->GetScreenSize(w, h);
	float velocity = g_LocalPlayer->m_vecVelocity().Length2D() + 0.5;
	static float groundvelocity = 0;
	if (g_LocalPlayer->m_fFlags() & FL_ONGROUND) {
		groundvelocity = g_LocalPlayer->m_vecVelocity().Length2D() + 0.5;
		Render::Get().RenderText(std::to_string((int)velocity), ImVec2(w / 2, h - h / 5 - 15), 15, Color(255, 255, 255), true);
	}
	else
		Render::Get().RenderText(std::to_string((int)velocity) + " (" + std::to_string((int)groundvelocity) + ")", ImVec2(w / 2, h - h / 5 - 15), 15, Color(255, 255, 255), true);
}
//--------------------------------------------------------------------------------
void Visuals::DrawKeyPresses() {
	int w, h;
	g_EngineClient->GetScreenSize(w, h);
	if (GetAsyncKeyState(int('W')))
		Render::Get().RenderText("W", ImVec2(w / 2, h - h / 5), 15.f, Color(255, 255, 255, 255), true);
	else
		Render::Get().RenderText("_", ImVec2(w / 2, h - h / 5), 15.f, Color(255, 255, 255, 255), true);

	if (GetAsyncKeyState(int('S')))
		Render::Get().RenderText("S", ImVec2(w / 2, h - h / 5 + 15), 15.f, Color(255, 255, 255, 255), true);
	else
		Render::Get().RenderText("_", ImVec2(w / 2, h - h / 5 + 15), 15.f, Color(255, 255, 255, 255), true);

	if (GetAsyncKeyState(int('A')))
		Render::Get().RenderText("A", ImVec2(w / 2 - 15, h - h / 5), 15.f, Color(255, 255, 255, 255), true);
	else
		Render::Get().RenderText("_", ImVec2(w / 2 - 15, h - h / 5), 15.f, Color(255, 255, 255, 255), true);

	if (GetAsyncKeyState(int('D')))
		Render::Get().RenderText("D", ImVec2(w / 2 + 15, h - h / 5), 15.f, Color(255, 255, 255, 255), true);
	else
		Render::Get().RenderText("_", ImVec2(w / 2 + 15, h - h / 5), 15.f, Color(255, 255, 255, 255), true);
}
//--------------------------------------------------------------------------------
void Visuals::ThirdPerson() {
	if (!g_LocalPlayer)
		return;

	if (g_LocalPlayer->IsAlive() && g_Options.misc_thirdperson || GetKeyState(g_Options.misc_thirdperson_hotkey))
	{
		if (!g_Input->m_fCameraInThirdPerson)
		{
			g_Input->m_fCameraInThirdPerson = true;
		}

		float dist = 150.f;

		QAngle *view = g_LocalPlayer->GetVAngles();
		trace_t tr;
		Ray_t ray;

		Vector desiredCamOffset = Vector(cos(DEG2RAD(view->yaw)) * dist,
			sin(DEG2RAD(view->yaw)) * dist,
			sin(DEG2RAD(-view->pitch)) * dist
		);

		//cast a ray from the Current camera Origin to the Desired 3rd person Camera origin
		ray.Init(g_LocalPlayer->GetEyePos(), (g_LocalPlayer->GetEyePos() - desiredCamOffset));
		CTraceFilter traceFilter;
		traceFilter.pSkip = g_LocalPlayer;
		g_EngineTrace->TraceRay(ray, MASK_SHOT, &traceFilter, &tr);

		Vector diff = g_LocalPlayer->GetEyePos() - tr.endpos;

		float distance2D = sqrt(abs(diff.x * diff.x) + abs(diff.y * diff.y));// Pythagorean

		bool horOK = distance2D > (dist - 2.0f);
		bool vertOK = (abs(diff.z) - abs(desiredCamOffset.z) < 3.0f);

		float cameraDistance;

		if (horOK && vertOK)  // If we are clear of obstacles
		{
			cameraDistance = dist; // go ahead and set the distance to the setting
		}
		else
		{
			if (vertOK) // if the Vertical Axis is OK
			{
				cameraDistance = distance2D * 0.95f;
			}
			else// otherwise we need to move closer to not go into the floor/ceiling
			{
				cameraDistance = abs(diff.z) * 0.95f;
			}
		}
		g_Input->m_fCameraInThirdPerson = true;

		g_Input->m_vecCameraOffset.z = cameraDistance;
	}
	else
	{
		g_Input->m_fCameraInThirdPerson = false;
	}
}


void Visuals::AddToDrawList() {
	for (auto i = 1; i <= g_EntityList->GetHighestEntityIndex(); ++i) {
		auto entity = C_BaseEntity::GetEntityByIndex(i);

		if (!entity)
			continue;
		
		if (entity == g_LocalPlayer && !g_Input->m_fCameraInThirdPerson)
			continue;

		if (i <= g_GlobalVars->maxClients) {
			auto player = Player();
			if (player.Begin((C_BasePlayer*)entity)) {
				if (g_Options.esp_player_boxes)     player.RenderBox(entity);
				if (g_Options.esp_player_weapons)   player.RenderWeaponName(entity);
				if (g_Options.esp_player_names)     player.RenderName(entity);
				if (g_Options.esp_player_health)    player.RenderHealth(entity);
				if (g_Options.esp_player_armour)    player.RenderArmour();
				if (g_Options.esp_player_ammo)		player.RenderAmmo(entity);
			}
		}
		else if (g_Options.esp_dropped_weapons && entity->IsWeapon())
			RenderWeapon(static_cast<C_BaseCombatWeapon*>(entity));
		else if (entity->IsPlantedC4() && g_Options.esp_planted_c4)
			RenderPlantedC4(entity);
		else if (entity->IsLoot() && g_Options.esp_items)
			RenderItemEsp(entity);
		else if (g_Options.misc_bomb_timer)
			BombTimer(entity);
	}
	if (g_Options.misc_spectators)
		Spectators();
}
