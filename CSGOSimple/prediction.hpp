#pragma once
#include "valve_sdk/csgostructs.hpp"
#include "helpers/math.hpp"

class CPredictionSystem : public Singleton<CPredictionSystem>
{

public:
	CPredictionSystem() {
		auto client = GetModuleHandleA("client.dll");
		predictionRandomSeed = *(int**)(Utils::PatternScan(client, "A3 ? ? ? ? 66 0F 6E 86") + 0x1);
		predictionPlayer = *reinterpret_cast<C_BasePlayer**>(Utils::PatternScan(client, "89 35 ? ? ? ? F3 0F 10 48") + 0x2);
	}

	void Start(CUserCmd* userCMD, C_BasePlayer* player);
	void End(C_BasePlayer* player);

	inline float GetOldCurTime() { return m_flOldCurTime; }

private:

	float m_flOldCurTime;
	float m_flOldFrametime;

	CMoveData moveData;

	int* predictionRandomSeed;
	C_BasePlayer* predictionPlayer;
};