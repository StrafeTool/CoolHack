#pragma once
#include "../helpers/math.hpp"
struct HitMarkerInfo
{
	float m_flExpTime;
	int m_iDmg;
};

class HitMarkerEvent : public IGameEventListener2, public Singleton<HitMarkerEvent>
{
public:
	void FireGameEvent(IGameEvent* event);
	int  GetEventDebugID(void);
	void RegisterSelf();
	void UnregisterSelf();
private:
	std::vector<HitMarkerInfo> hitMarkerInfo;
};