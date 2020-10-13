#include "hooks.hpp"
#include <intrin.h>  

#include "render.hpp"
#include "menu.hpp"
#include "options.hpp"
#include "helpers/input.hpp"
#include "helpers/utils.hpp"
#include "features/Misc.hpp"
#include "features/chams.hpp"
#include "features/visuals.hpp"
#include "features/glow.hpp"
#include "features/backtrack.hpp"
#include "prediction.hpp"

#pragma intrinsic(_ReturnAddress)  
#define MAX_COORD_FLOAT ( 16384.0f )
#define MIN_COORD_FLOAT ( -MAX_COORD_FLOAT )

namespace Hooks {

	static bool __stdcall shouldDrawFog() noexcept
	{
		return false;
	}

	void Initialize()
	{
		hlclient_hook.setup(g_CHLClient);
		direct3d_hook.setup(g_D3DDevice9);
		vguipanel_hook.setup(g_VGuiPanel);
		vguisurf_hook.setup(g_VGuiSurface);
		sound_hook.setup(g_EngineSound);
		mdlrender_hook.setup(g_MdlRender);
		clientmode_hook.setup(g_ClientMode);
		ConVar* sv_cheats_con = g_CVar->FindVar("sv_cheats");
		sv_cheats.setup(sv_cheats_con);
		bsp_query_hook.setup(g_EngineClient->GetBSPTreeQuery());

		direct3d_hook.hook_index(index::EndScene, hkEndScene);
		direct3d_hook.hook_index(index::Reset, hkReset);
		hlclient_hook.hook_index(index::FrameStageNotify, hkFrameStageNotify);
		hlclient_hook.hook_index(index::CreateMove, hkCreateMove_Proxy);
		vguipanel_hook.hook_index(index::PaintTraverse, hkPaintTraverse);
		sound_hook.hook_index(index::EmitSound1, hkEmitSound1);
		vguisurf_hook.hook_index(index::LockCursor, hkLockCursor);
		mdlrender_hook.hook_index(index::DrawModelExecute, hkDrawModelExecute);
		clientmode_hook.hook_index(index::DoPostScreenSpaceEffects, hkDoPostScreenEffects);
		clientmode_hook.hook_index(index::OverrideView, hkOverrideView);
		sv_cheats.hook_index(index::SvCheatsGetBool, hkSvCheatsGetBool);
		clientmode_hook.hook_index(index::DrawFog, shouldDrawFog); // Remove Fog
		bsp_query_hook.hook_index(index::ListLeavesInBox, hkListLeavesInBox); // Far Chams
	}
	//--------------------------------------------------------------------------------
	void Shutdown()
	{
		hlclient_hook.unhook_all();
		direct3d_hook.unhook_all();
		vguipanel_hook.unhook_all();
		vguisurf_hook.unhook_all();
		mdlrender_hook.unhook_all();
		clientmode_hook.unhook_all();
		sound_hook.unhook_all();
		sv_cheats.unhook_all();

		Glow::Get().Shutdown();
	}
	//--------------------------------------------------------------------------------
	struct RenderableInfo_t {
		IClientRenderable* m_pRenderable;
		void* m_pAlphaProperty;
		int m_EnumCount;
		int m_nRenderFrame;
		unsigned short m_FirstShadow;
		unsigned short m_LeafList;
		short m_Area;
		uint16_t m_Flags;   // 0x0016
		uint16_t m_Flags2; // 0x0018
		Vector m_vecBloatedAbsMins;
		Vector m_vecBloatedAbsMaxs;
		Vector m_vecAbsMins;
		Vector m_vecAbsMaxs;
		int pad;
	};
	//--------------------------------------------------------------------------------
	int __fastcall hkListLeavesInBox(void* bsp, void* edx, Vector& mins, Vector& maxs, unsigned short* pList, int listMax) {
		typedef int(__thiscall* ListLeavesInBox)(void*, const Vector&, const Vector&, unsigned short*, int);
		static auto ofunc = bsp_query_hook.get_original< ListLeavesInBox >(index::ListLeavesInBox);

		if (!g_Options.chams || *(uint32_t*)_ReturnAddress() != 0x14244489)
			return ofunc(bsp, mins, maxs, pList, listMax);

		auto info = *(RenderableInfo_t**)((uintptr_t)_AddressOfReturnAddress() + 0x14);
		if (!info || !info->m_pRenderable)
			return ofunc(bsp, mins, maxs, pList, listMax);

		auto base_entity = info->m_pRenderable->GetIClientUnknown()->GetBaseEntity();
		if (!base_entity || !base_entity->IsPlayer())
			return ofunc(bsp, mins, maxs, pList, listMax);

		info->m_Flags &= ~0x100;
		info->m_Flags2 |= 0xC0;

		static const Vector map_min = Vector(MIN_COORD_FLOAT, MIN_COORD_FLOAT, MIN_COORD_FLOAT);
		static const Vector map_max = Vector(MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT);
		auto count = ofunc(bsp, map_min, map_max, pList, listMax);
		return count;
	}
	//--------------------------------------------------------------------------------
	long __stdcall hkEndScene(IDirect3DDevice9* pDevice)
	{
		static auto oEndScene = direct3d_hook.get_original<decltype(&hkEndScene)>(index::EndScene);
		static auto viewmodel_fov = g_CVar->FindVar("viewmodel_fov");
		static auto viewmodel_offset_x = g_CVar->FindVar("viewmodel_offset_x");
		static auto viewmodel_offset_y = g_CVar->FindVar("viewmodel_offset_y");
		static auto viewmodel_offset_z = g_CVar->FindVar("viewmodel_offset_z");
		viewmodel_fov->m_fnChangeCallbacks.m_Size = 0;
		viewmodel_fov->SetValue(g_Options.viewmodel_fov);	
		viewmodel_offset_x->m_fnChangeCallbacks.m_Size = 0;
		viewmodel_offset_x->SetValue(g_Options.viewmodel_offset_x);
		viewmodel_offset_y->m_fnChangeCallbacks.m_Size = 0;
		viewmodel_offset_y->SetValue(g_Options.viewmodel_offset_y);
		viewmodel_offset_z->m_fnChangeCallbacks.m_Size = 0;
		viewmodel_offset_z->SetValue(g_Options.viewmodel_offset_z);

		DWORD colorwrite, srgbwrite;
		IDirect3DVertexDeclaration9* vert_dec = nullptr;
		IDirect3DVertexShader9* vert_shader = nullptr;
		DWORD dwOld_D3DRS_COLORWRITEENABLE = NULL;
		pDevice->GetRenderState(D3DRS_COLORWRITEENABLE, &colorwrite);
		pDevice->GetRenderState(D3DRS_SRGBWRITEENABLE, &srgbwrite);
		pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
		pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
		pDevice->GetRenderState(D3DRS_COLORWRITEENABLE, &dwOld_D3DRS_COLORWRITEENABLE);
		pDevice->GetVertexDeclaration(&vert_dec);
		pDevice->GetVertexShader(&vert_shader);
		pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
		pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
		pDevice->SetSamplerState(NULL, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		pDevice->SetSamplerState(NULL, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		pDevice->SetSamplerState(NULL, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
		pDevice->SetSamplerState(NULL, D3DSAMP_SRGBTEXTURE, NULL);		
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		auto esp_drawlist = Render::Get().RenderScene();
		Menu::Get().Render();	
		ImGui::Render(esp_drawlist);
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, colorwrite);
		pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, srgbwrite);
		pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, dwOld_D3DRS_COLORWRITEENABLE);
		pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, true);
		pDevice->SetVertexDeclaration(vert_dec);
		pDevice->SetVertexShader(vert_shader);
		return oEndScene(pDevice);
	}
	//--------------------------------------------------------------------------------
	long __stdcall hkReset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters)
	{
		static auto oReset = direct3d_hook.get_original<decltype(&hkReset)>(index::Reset);

		Menu::Get().OnDeviceLost();

		auto hr = oReset(device, pPresentationParameters);

		if (hr >= 0)
			Menu::Get().OnDeviceReset();

		return hr;
	}
	//--------------------------------------------------------------------------------
	void __stdcall hkCreateMove(int sequence_number, float input_sample_frametime, bool active, bool& bSendPacket)
	{
		static auto oCreateMove = hlclient_hook.get_original<decltype(&hkCreateMove_Proxy)>(index::CreateMove);

		oCreateMove(g_CHLClient, 0, sequence_number, input_sample_frametime, active);

		auto cmd = g_Input->GetUserCmd(sequence_number);
		auto verified = g_Input->GetVerifiedCmd(sequence_number);

		if (!cmd || !cmd->command_number)
			return;
		QAngle oldAngle = cmd->viewangles;

		if (Menu::Get().IsVisible())
			cmd->buttons &= ~IN_ATTACK;

		if (g_Options.misc_bhop)
			Misc::Bhop(cmd);

		if (g_Options.misc_rcs)
			Misc::RCS(cmd);

		if (g_Options.misc_silentwalk)
			Misc::SilentWalk(cmd);


		if (g_Options.misc_desync)
		{
			Misc::Fakelag(cmd, bSendPacket);
			Misc::Desync(cmd, bSendPacket);
			Misc::Desync(cmd, bSendPacket);
		}	

		if (cmd->buttons & IN_SCORE)
			g_CHLClient->DispatchUserMessage(CS_UM_ServerRankRevealAll, 0, 0, nullptr);

		TimeWarp().Get().CreateMove(cmd);

		CPredictionSystem::Get().Start(cmd, g_LocalPlayer); {
			if (g_Options.misc_jumpbug)
				Misc::Jumpbug(cmd);
		}
		CPredictionSystem::Get().End(g_LocalPlayer);

		Math::ClampAngles(cmd->viewangles);
		Math::Normalize3(cmd->viewangles);
		if (g_Options.misc_desync)
			Misc::MovementFix(cmd, oldAngle, cmd->viewangles);
		cmd->buttons |= IN_BULLRUSH;
		verified->m_cmd = *cmd;
		verified->m_crc = cmd->GetChecksum();
	}
	//--------------------------------------------------------------------------------
	__declspec(naked) void __fastcall hkCreateMove_Proxy(void* _this, int, int sequence_number, float input_sample_frametime, bool active)
	{
		__asm
		{
			push ebp
			mov  ebp, esp
			push ebx; not sure if we need this
			push esp
			push dword ptr[active]
			push dword ptr[input_sample_frametime]
			push dword ptr[sequence_number]
			call Hooks::hkCreateMove
			pop  ebx
			pop  ebp
			retn 0Ch
		}
	}
	//--------------------------------------------------------------------------------
	void __fastcall hkPaintTraverse(void* _this, int edx, vgui::VPANEL panel, bool forceRepaint, bool allowForce)
	{
		static auto panelId = vgui::VPANEL{ 0 };
		static auto oPaintTraverse = vguipanel_hook.get_original<decltype(&hkPaintTraverse)>(index::PaintTraverse);
		
		if (g_Options.misc_remove_scope) {
			if (strstr(g_VGuiPanel->GetName(panel), "HudZoom")) {
				if (g_EngineClient->IsConnected() && g_EngineClient->IsInGame())
					return;
			}
		}

		oPaintTraverse(g_VGuiPanel, edx, panel, forceRepaint, allowForce);

		if (!panelId) {
			const auto panelName = g_VGuiPanel->GetName(panel);
			if (!strcmp(panelName, "FocusOverlayPanel")) {
				panelId = panel;
			}
		}
		else if (panelId == panel) 
		{
			//Ignore 50% cuz it called very often
			static bool bSkip = false;
			bSkip = !bSkip;

			if (bSkip)
				return;

			Render::Get().BeginScene();
		}
	}
	//--------------------------------------------------------------------------------
	void __fastcall hkEmitSound1(void* _this, int edx, IRecipientFilter& filter, int iEntIndex, int iChannel, const char* pSoundEntry, unsigned int nSoundEntryHash, const char *pSample, float flVolume, int nSeed, float flAttenuation, int iFlags, int iPitch, const Vector* pOrigin, const Vector* pDirection, void* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity, int unk) {
		static auto ofunc = sound_hook.get_original<decltype(&hkEmitSound1)>(index::EmitSound1);


		if (!strcmp(pSoundEntry, "UIPanorama.popup_accept_match_beep")) {
			static auto fnAccept = reinterpret_cast<bool(__stdcall*)(const char*)>(Utils::PatternScan(GetModuleHandleA("client.dll"), "55 8B EC 83 E4 F8 8B 4D 08 BA ? ? ? ? E8 ? ? ? ? 85 C0 75 12"));

			if (fnAccept) {

				fnAccept("");

				//This will flash the CSGO window on the taskbar
				//so we know a game was found (you cant hear the beep sometimes cause it auto-accepts too fast)
				FLASHWINFO fi;
				fi.cbSize = sizeof(FLASHWINFO);
				fi.hwnd = InputSys::Get().GetMainWindow();
				fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
				fi.uCount = 0;
				fi.dwTimeout = 0;
				FlashWindowEx(&fi);
			}
		}

		ofunc(g_EngineSound, edx, filter, iEntIndex, iChannel, pSoundEntry, nSoundEntryHash, pSample, flVolume, nSeed, flAttenuation, iFlags, iPitch, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity, unk);

	}
	//--------------------------------------------------------------------------------
	int __fastcall hkDoPostScreenEffects(void* _this, int edx, int a1)
	{
		static auto oDoPostScreenEffects = clientmode_hook.get_original<decltype(&hkDoPostScreenEffects)>(index::DoPostScreenSpaceEffects);

		if (g_LocalPlayer && g_Options.glow)
			Glow::Get().Run();

		return oDoPostScreenEffects(g_ClientMode, edx, a1);
	}
	//--------------------------------------------------------------------------------
	void __fastcall hkFrameStageNotify(void* _this, int edx, ClientFrameStage_t stage)
	{
		int w, h;
		g_EngineClient->GetScreenSize(w, h);
		static auto full_bright = g_CVar->FindVar("mat_fullbright");
		static auto show_impacts = g_CVar->FindVar("sv_showimpacts");
		static auto aspect_ratio = g_CVar->FindVar("r_aspectratio");
		static auto postprocess = g_CVar->FindVar("mat_postprocess_enable");
		static auto crosshair = g_CVar->FindVar("weapon_debug_spread_show");
		static auto ofunc = hlclient_hook.get_original<decltype(&hkFrameStageNotify)>(index::FrameStageNotify);
		static auto blur = g_MatSystem->FindMaterial("dev/scope_bluroverlay");
		static auto scope_dot_green = g_MatSystem->FindMaterial("models/weapons/shared/scope/scope_dot_red");
		static auto scope_dot_red = g_MatSystem->FindMaterial("models/weapons/shared/scope/scope_dot_green");
		static auto panorama_blur = g_CVar->FindVar("@panorama_disable_blur");
		if (g_EngineClient->IsInGame() && g_LocalPlayer)
		{
			float aspect = (w / h);
			float ratio = (g_Options.misc_aspect_ratio / 2);
			if (g_Options.misc_aspect_ratio > 0 || g_Options.misc_aspect_ratio < 1)
				aspect_ratio->SetValue(ratio);
			else
				aspect_ratio->SetValue(0);
		
			full_bright->SetValue(g_Options.misc_fullbright);

			//if (g_Options.misc_showimpacts)
			//	show_impacts->SetValue(g_Options.misc_showimpacts);

			if (g_Options.misc_remove_scope) {
				blur->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, stage == FRAME_RENDER_START);
				scope_dot_green->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, stage == FRAME_RENDER_START);
				scope_dot_red->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, stage == FRAME_RENDER_START);
				panorama_blur->SetValue(1);
				postprocess->SetValue(0);
				if (g_LocalPlayer->m_bIsScoped())
					crosshair->SetValue(0);
				else
					crosshair->SetValue(3);
			}
			if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START)
				Visuals::Get().SkinChanger();

			Visuals::Get().Nightmode();
		}
		ofunc(g_CHLClient, edx, stage);
	}
	//--------------------------------------------------------------------------------
	void __fastcall hkOverrideView(void* _this, int edx, CViewSetup* vsView)
	{
		static auto ofunc = clientmode_hook.get_original<decltype(&hkOverrideView)>(index::OverrideView);

		if (g_EngineClient->IsInGame() && vsView)
			Visuals::Get().ThirdPerson();

		ofunc(g_ClientMode, edx, vsView);
	}
	//--------------------------------------------------------------------------------
	void __fastcall hkLockCursor(void* _this)
	{
		static auto ofunc = vguisurf_hook.get_original<decltype(&hkLockCursor)>(index::LockCursor);

		if (Menu::Get().IsVisible()) {
			g_VGuiSurface->UnlockCursor();
			g_InputSystem->ResetInputState();
			return;
		}
		ofunc(g_VGuiSurface);

	}
	//--------------------------------------------------------------------------------
	void __fastcall hkDrawModelExecute(void* _this, int edx, IMatRenderContext* ctx, const DrawModelState_t& state, const ModelRenderInfo_t& pInfo, matrix3x4_t* pCustomBoneToWorld)
	{
		static auto ofunc = mdlrender_hook.get_original<decltype(&hkDrawModelExecute)>(index::DrawModelExecute);

		if (g_MdlRender->IsForcedMaterialOverride() &&
			!strstr(pInfo.pModel->szName, "arms") &&
			!strstr(pInfo.pModel->szName, "weapons/v_")) {
			return ofunc(_this, edx, ctx, state, pInfo, pCustomBoneToWorld);
		}

		Chams::Get().OnDrawModelExecute(ctx, state, pInfo, pCustomBoneToWorld);
		ofunc(_this, edx, ctx, state, pInfo, pCustomBoneToWorld);
		g_MdlRender->ForcedMaterialOverride(nullptr);
	}
	
	bool __fastcall hkSvCheatsGetBool(PVOID pConVar, void* edx)
	{
		static auto dwCAM_Think = Utils::PatternScan(GetModuleHandleW(L"client.dll"), "85 C0 75 30 38 86");
		static auto ofunc = sv_cheats.get_original<bool(__thiscall *)(PVOID)>(13);
		if (!ofunc)
			return false;

		if (reinterpret_cast<DWORD>(_ReturnAddress()) == reinterpret_cast<DWORD>(dwCAM_Think))
			return true;
		return ofunc(pConVar);
	}
}
