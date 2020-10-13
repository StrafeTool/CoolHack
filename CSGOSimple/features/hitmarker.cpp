#include "hitmarker.hpp"
#include "../features/visuals.hpp"
#include "../options.hpp"
#pragma comment(lib, "winmm.lib")

void HitMarkerEvent::FireGameEvent(IGameEvent* event)
{
	if (!event)
		return;

	if (g_Options.misc_hitmarker)
	{
		if (g_EngineClient->GetPlayerForUserID(event->GetInt("attacker")) == g_EngineClient->GetLocalPlayer() &&
			g_EngineClient->GetPlayerForUserID(event->GetInt("userid")) != g_EngineClient->GetLocalPlayer())
		{
			hitMarkerInfo.push_back({ g_GlobalVars->curtime + 0.8f, event->GetInt("dmg_health") });
			g_EngineClient->ExecuteClientCmd("play weapons\\smokegrenade\\grenade_hit1.wav");
		}
	}
}

int HitMarkerEvent::GetEventDebugID(void)
{
	return EVENT_DEBUG_ID_INIT;
}

void HitMarkerEvent::RegisterSelf()
{
	g_GameEvents->AddListener(this, "player_hurt", false);
}

void HitMarkerEvent::UnregisterSelf()
{
	g_GameEvents->RemoveListener(this);
}