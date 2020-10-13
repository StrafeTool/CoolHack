#include "chams.hpp"
#include <fstream>

#include "../valve_sdk/csgostructs.hpp"
#include "../options.hpp"
#include "../hooks.hpp"
#include "../helpers/input.hpp"

void Chams::OverrideMaterial(bool ignoreZ,  const Color& rgba) {
	IMaterial* material = nullptr; 
	material = materialFlat = g_MatSystem->FindMaterial("debug/debugdrawflat");
	if (material == nullptr)
		material->IncrementReferenceCount();

	material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, ignoreZ);
	material->AlphaModulate(rgba.a() / 255.0f);
	material->ColorModulate(rgba.r() / 255.0f,rgba.g() / 255.0f,rgba.b() / 255.0f);
	g_MdlRender->ForcedMaterialOverride(material);
}

void Chams::OverrideMaterialGlow(bool ignoreZ, const Color& rgba) {
	IMaterial* material = nullptr;
	material = materialArmRace = g_MatSystem->FindMaterial("dev/glow_armsrace.vmt");
	material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, ignoreZ);
	material->AlphaModulate(rgba.a() / 255.0f);
	material->ColorModulate(rgba.r() / 255.0f, rgba.g() / 255.0f, rgba.b() / 255.0f);
	g_MdlRender->ForcedMaterialOverride(material);
}

void Chams::OnDrawModelExecute(IMatRenderContext* ctx, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* matrix)
{
	static auto fnDME = Hooks::mdlrender_hook.get_original<decltype(&Hooks::hkDrawModelExecute)>(index::DrawModelExecute);
	const auto mdl = info.pModel;

	bool is_arm = strstr(mdl->szName, "arms") != nullptr;
	bool is_player = strstr(mdl->szName, "models/player") != nullptr;
	bool is_sleeve = strstr(mdl->szName, "sleeve") != nullptr;
	bool is_weapon = strstr(mdl->szName, "weapons/v_")  != nullptr;

	const auto clr_front = g_Options.color_chams_visible;
	const auto clr_back = g_Options.color_chams_occluded;

	if (is_player && g_Options.chams) {

		auto ent = C_BasePlayer::GetPlayerByIndex(info.entity_index);

		if (ent && g_LocalPlayer && ent->IsAlive()) {
			const auto enemy = ent->m_iTeamNum() != g_LocalPlayer->m_iTeamNum();
			if (!enemy && g_Options.enemies_only)
				return;

			if (g_Options.chams_xyz) {
				OverrideMaterial(true, clr_back);
				fnDME(g_MdlRender, 0, ctx, state, info, matrix);
				OverrideMaterial(false, clr_front);
			}
			else
				OverrideMaterial(false, clr_front);
		}
	} 
	if (is_weapon && g_Options.misc_remove_scope) {
		if (g_LocalPlayer->m_bIsScoped()) {
			OverrideMaterial(false, Color(0, 0, 0, 0));
		}
	}
}