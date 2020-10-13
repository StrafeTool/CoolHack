#pragma once

#include "../singleton.hpp"

class IMatRenderContext;
struct DrawModelState_t;
struct ModelRenderInfo_t;
class matrix3x4_t;
class IMaterial;
class Color;

class Chams : public Singleton<Chams>
{
public:
	void OnDrawModelExecute(IMatRenderContext* ctx, const DrawModelState_t &state, const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld);

private:
    void OverrideMaterial(bool ignoreZ, const Color& rgba);
    void OverrideMaterialGlow(bool ignoreZ, const Color& rgba);
    IMaterial* materialFlat = nullptr;
    IMaterial* materialArmRace = nullptr;
};