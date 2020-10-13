#include "../valve_sdk/csgostructs.hpp"
#include "../helpers/math.hpp"
#include <deque>
#define NUM_OF_TICKS 13

struct StoredData
{
	float simtime;
	Vector hitboxPos;
};
class TimeWarp : public Singleton<TimeWarp>
{
	int nLatestTick;

public:
	StoredData TimeWarpData[64][NUM_OF_TICKS];
	void CreateMove(CUserCmd* cmd);
};

