#include "Misc.hpp"
#include <algorithm>

void Misc::Bhop(CUserCmd* cmd)
{
  static bool jumped_last_tick = false;
  static bool should_fake_jump = false;

  if (!g_LocalPlayer)
	  return;

  if (!g_LocalPlayer->IsAlive())
	  return;

  if (g_LocalPlayer->m_nMoveType() == MOVETYPE_LADDER || g_LocalPlayer->m_nMoveType() == MOVETYPE_NOCLIP)
	  return;

  if (g_LocalPlayer->m_fFlags() & FL_INWATER)
	  return;

  if(!jumped_last_tick && should_fake_jump) {
    should_fake_jump = false;
    cmd->buttons |= IN_JUMP;
  } else if(cmd->buttons & IN_JUMP) {
    if(g_LocalPlayer->m_fFlags() & FL_ONGROUND) {
      jumped_last_tick = true;
      should_fake_jump = true;
    } else {
      cmd->buttons &= ~IN_JUMP;
      jumped_last_tick = false;
    }
  } else {
    jumped_last_tick = false;
    should_fake_jump = false;
  }
}

void Misc::RCS(CUserCmd* cmd)
{
    if (!g_LocalPlayer)
        return;

    static QAngle v_old_punch(0, 0, 0);
    if (g_LocalPlayer->m_iShotsFired() > 1) {
        QAngle aim_punch = g_LocalPlayer->m_aimPunchAngle() * 2.0f;
        QAngle v_rcs = cmd->viewangles - (aim_punch - v_old_punch);
        Math::ClampAngles(v_rcs);
        Math::Normalize3(v_rcs);
        g_EngineClient->SetViewAngles(&v_rcs);
        v_old_punch = aim_punch;
    }
    else v_old_punch.pitch = v_old_punch.yaw = v_old_punch.roll = 0;
}
//--------------------------------------------------------------------------------
float scaleDamageArmor(float flDamage, int armor_value)
{
    float flArmorRatio = 0.5f;
    float flArmorBonus = 0.5f;
    if (armor_value > 0) {
        float flNew = flDamage * flArmorRatio;
        float flArmor = (flDamage - flNew) * flArmorBonus;

        if (flArmor > static_cast<float>(armor_value)) {
            flArmor = static_cast<float>(armor_value) * (1.f / flArmorBonus);
            flNew = flDamage - flArmor;
        }

        flDamage = flNew;
    }
    return flDamage;
}
//--------------------------------------------------------------------------------
void Misc::SilentWalk(CUserCmd* cmd)
{
    Vector moveDir = Vector(0.f, 0.f, 0.f);
    float maxSpeed = 130.f;
    int movetype = g_LocalPlayer->m_nMoveType();
    bool InAir = !(g_LocalPlayer->m_fFlags() & FL_ONGROUND);
    if (movetype == MOVETYPE_FLY || movetype == MOVETYPE_NOCLIP || InAir || cmd->buttons & IN_DUCK || !(cmd->buttons & IN_SPEED))
        return;
    moveDir.x = cmd->sidemove;
    moveDir.y = cmd->forwardmove;
    moveDir = Math::ClampMagnitude(moveDir, maxSpeed);
    cmd->sidemove = moveDir.x;
    cmd->forwardmove = moveDir.y;
    if (!(g_LocalPlayer->m_vecVelocity().Length2D() > maxSpeed + 1))
        cmd->buttons &= ~IN_SPEED;
}
//--------------------------------------------------------------------------------
void Misc::MovementFix(CUserCmd* m_Cmd, QAngle wish_angle, QAngle old_angles) {
    if (old_angles.pitch != wish_angle.pitch || old_angles.yaw != wish_angle.yaw || old_angles.roll != wish_angle.roll) {
        Vector wish_forward, wish_right, wish_up, cmd_forward, cmd_right, cmd_up;

        auto viewangles = old_angles;
        auto movedata = Vector(m_Cmd->forwardmove, m_Cmd->sidemove, m_Cmd->upmove);
        viewangles.Normalize();

        if (!(g_LocalPlayer->m_fFlags() & FL_ONGROUND) && viewangles.roll != 0.f)
            movedata.y = 0.f;

        Math::AngleVectors(wish_angle, wish_forward, wish_right, wish_up);
        Math::AngleVectors(viewangles, cmd_forward, cmd_right, cmd_up);

        auto v8 = sqrt(wish_forward.x * wish_forward.x + wish_forward.y * wish_forward.y), v10 = sqrt(wish_right.x * wish_right.x + wish_right.y * wish_right.y), v12 = sqrt(wish_up.z * wish_up.z);

        Vector wish_forward_norm(1.0f / v8 * wish_forward.x, 1.0f / v8 * wish_forward.y, 0.f),
            wish_right_norm(1.0f / v10 * wish_right.x, 1.0f / v10 * wish_right.y, 0.f),
            wish_up_norm(0.f, 0.f, 1.0f / v12 * wish_up.z);

        auto v14 = sqrt(cmd_forward.x * cmd_forward.x + cmd_forward.y * cmd_forward.y), v16 = sqrt(cmd_right.x * cmd_right.x + cmd_right.y * cmd_right.y), v18 = sqrt(cmd_up.z * cmd_up.z);

        Vector cmd_forward_norm(1.0f / v14 * cmd_forward.x, 1.0f / v14 * cmd_forward.y, 1.0f / v14 * 0.0f),
            cmd_right_norm(1.0f / v16 * cmd_right.x, 1.0f / v16 * cmd_right.y, 1.0f / v16 * 0.0f),
            cmd_up_norm(0.f, 0.f, 1.0f / v18 * cmd_up.z);

        auto v22 = wish_forward_norm.x * movedata.x, v26 = wish_forward_norm.y * movedata.x, v28 = wish_forward_norm.z * movedata.x, v24 = wish_right_norm.x * movedata.y, v23 = wish_right_norm.y * movedata.y, v25 = wish_right_norm.z * movedata.y, v30 = wish_up_norm.x * movedata.z, v27 = wish_up_norm.z * movedata.z, v29 = wish_up_norm.y * movedata.z;

        Vector correct_movement;
        correct_movement.x = cmd_forward_norm.x * v24 + cmd_forward_norm.y * v23 + cmd_forward_norm.z * v25 + (cmd_forward_norm.x * v22 + cmd_forward_norm.y * v26 + cmd_forward_norm.z * v28) + (cmd_forward_norm.y * v30 + cmd_forward_norm.x * v29 + cmd_forward_norm.z * v27);
        correct_movement.y = cmd_right_norm.x * v24 + cmd_right_norm.y * v23 + cmd_right_norm.z * v25 + (cmd_right_norm.x * v22 + cmd_right_norm.y * v26 + cmd_right_norm.z * v28) + (cmd_right_norm.x * v29 + cmd_right_norm.y * v30 + cmd_right_norm.z * v27);
        correct_movement.z = cmd_up_norm.x * v23 + cmd_up_norm.y * v24 + cmd_up_norm.z * v25 + (cmd_up_norm.x * v26 + cmd_up_norm.y * v22 + cmd_up_norm.z * v28) + (cmd_up_norm.x * v30 + cmd_up_norm.y * v29 + cmd_up_norm.z * v27);

        correct_movement.x = std::clamp(correct_movement.x, -450.f, 450.f);
        correct_movement.y = std::clamp(correct_movement.y, -450.f, 450.f);
        correct_movement.z = std::clamp(correct_movement.z, -320.f, 320.f);

        m_Cmd->forwardmove = correct_movement.x;
        m_Cmd->sidemove = correct_movement.y;
        m_Cmd->upmove = correct_movement.z;

        m_Cmd->buttons &= ~(IN_MOVERIGHT | IN_MOVELEFT | IN_BACK | IN_FORWARD);
        if (m_Cmd->sidemove != 0.0) {
            if (m_Cmd->sidemove <= 0.0)
                m_Cmd->buttons |= IN_MOVELEFT;
            else
                m_Cmd->buttons |= IN_MOVERIGHT;
        }

        if (m_Cmd->forwardmove != 0.0) {
            if (m_Cmd->forwardmove <= 0.0)
                m_Cmd->buttons |= IN_BACK;
            else
                m_Cmd->buttons |= IN_FORWARD;
        }
    }
}
//--------------------------------------------------------------------------------
void Misc::Fakelag(CUserCmd* cmd, bool& bSendPacket) {
    if (g_EngineClient->IsVoiceRecording())
        return;
    if (!g_LocalPlayer)
        return;

    int chockepack = 0;
    auto NetChannel = g_EngineClient->GetNetChannel();
    if (!NetChannel)
        return;
    bSendPacket = true;

    chockepack = 1.f;
    bSendPacket = (NetChannel->m_nChokedPackets >= chockepack);
}
//--------------------------------------------------------------------------------
void Misc::Triggerbot(CUserCmd* cmd) {  
    if (!g_LocalPlayer->IsAlive() || !g_Options.misc_triggerbot)
        return;

    auto pWeapon = g_LocalPlayer->m_hActiveWeapon();

    if (!pWeapon)
        return;

    static bool enable = false;
    static int	key = 0;
    static bool head = false;
    static bool arms = false;
    static bool chest = false;
    static bool stomach = false;
    static bool legs = false;
   
    if (pWeapon->CanFire()) enable = true;
    if (pWeapon->CanFire()) key = true;
    if (pWeapon->CanFire()) head = true;
    if (pWeapon->CanFire()) arms = true;
    if (pWeapon->CanFire()) chest = true;
    if (pWeapon->CanFire()) stomach = true;
    if (pWeapon->CanFire()) legs = true;

    Vector src, dst, forward;
    trace_t tr;
    Ray_t ray;
    CTraceFilter filter;

    QAngle viewangle = cmd->viewangles;

    viewangle += g_LocalPlayer->m_aimPunchAngle() * 2.f;

    Math::AngleVectors(viewangle, forward);

    forward *= g_LocalPlayer->m_hActiveWeapon()->GetCSWeaponData()->flRangeModifier;
    filter.pSkip = g_LocalPlayer;
    src = g_LocalPlayer->GetEyePos();
    dst = src + forward;
    ray.Init(src, dst);

    g_EngineTrace->TraceRay(ray, 0x46004003, &filter, &tr);
    if (!tr.hit_entity)
        return;

    int hitgroup = tr.hitgroup;
    bool didHit = false;
    if ((head && tr.hitgroup == 1)
        || (chest && tr.hitgroup == 2)
        || (stomach && tr.hitgroup == 3)
        || (arms && (tr.hitgroup == 4 || tr.hitgroup == 5))
        || (legs && (tr.hitgroup == 6 || tr.hitgroup == 7)))
    {
        didHit = true;
    }

    auto can_hit = [&]() {
        auto chance_to_hit = (100.0f - 40) * 0.65f * 0.01125f;
        return !(pWeapon->GetInaccuracy() >= chance_to_hit);
    };
    if (can_hit())
    {
        if (didHit && (tr.hit_entity->GetBaseEntity()->m_iTeamNum() != g_LocalPlayer->m_iTeamNum()))
        {
            if (g_Options.misc_triggerbot_hotkey > 0 && GetAsyncKeyState(g_Options.misc_triggerbot_hotkey) & 0x8000 && enable)
            {
                cmd->buttons |= IN_ATTACK;
            }
        }
    }
}
//--------------------------------------------------------------------------------
bool break_lby = false;
float next_update = 0;
void Misc::UpdateLBY(CUserCmd* cmd, bool& bSendPacket) {
    float server_time = g_GlobalVars->interval_per_tick * g_LocalPlayer->m_nTickBase();
    float speed = g_LocalPlayer->m_vecVelocity().LengthSqr();

    if (speed > 0.1)
        next_update = server_time + 0.22;  

    break_lby = false;

    if (next_update <= server_time) {
        next_update = server_time + 1.1;
        break_lby = true;
    }

    if (!(g_LocalPlayer->m_fFlags() & FL_ONGROUND))
        break_lby = false;

    if (break_lby)
    {
        bSendPacket = false;
        cmd->viewangles.yaw += g_LocalPlayer->m_flLowerBodyYawTarget();//120.f;
    }
}
//--------------------------------------------------------------------------------
void Misc::Desync(CUserCmd* cmd, bool& bSendPacket) {
    //auto weapon = g_LocalPlayer->m_hActiveWeapon().Get();
    //if (!weapon) return;
    //if (!weapon->GetCSWeaponData()) return;
    //if (weapon->IsGrenade()) return;
    if (cmd->buttons & IN_USE || cmd->buttons & IN_ATTACK)
        return;

    static QAngle LastRealAngle = QAngle(0, 0, 0);
    if (!bSendPacket) {
        if (GetKeyState(VK_XBUTTON1))
            cmd->viewangles.yaw += 120.f;
        else
            cmd->viewangles.yaw -= 120.f;
    }
    else
        LastRealAngle = cmd->viewangles;
}

void Misc::Jumpbug(CUserCmd* cmd) {
    float max_radias = D3DX_PI * 2;
    float step = max_radias / 128;
    float xThick = 23;
    static bool bDidJump = true;
    static bool unduck = true;

    if (!GetAsyncKeyState(g_Options.misc_jumpbug_hotkey))
        return;

    if (g_LocalPlayer->m_fFlags() & (1 << 0)) {
        if (unduck) {
            bDidJump = false;
            cmd->buttons &= ~IN_DUCK; // duck
            cmd->buttons |= IN_JUMP; // jump
            unduck = false;
        }
        Vector pos = g_LocalPlayer->m_vecOrigin();
        for (float a = 0.f; a < max_radias; a += step) {
            Vector pt;
            pt.x = (xThick * cos(a)) + pos.x;
            pt.y = (xThick * sin(a)) + pos.y;
            pt.z = pos.z;


            Vector pt2 = pt;
            pt2.z -= 6;

            trace_t fag;

            Ray_t ray;
            ray.Init(pt, pt2);

            CTraceFilter flt;
            flt.pSkip = g_LocalPlayer;
            g_EngineTrace->TraceRay(ray, MASK_SOLID_BRUSHONLY, &flt, &fag);

            if (fag.fraction != 1.f && fag.fraction != 0.f) {
                bDidJump = true;
                cmd->buttons |= IN_DUCK; // duck
                cmd->buttons &= ~IN_JUMP; // jump
                unduck = true;
            }
        }
        for (float a = 0.f; a < max_radias; a += step) {
            Vector pt;
            pt.x = ((xThick - 2.f) * cos(a)) + pos.x;
            pt.y = ((xThick - 2.f) * sin(a)) + pos.y;
            pt.z = pos.z;

            Vector pt2 = pt;
            pt2.z -= 6;

            trace_t fag;

            Ray_t ray;
            ray.Init(pt, pt2);

            CTraceFilter flt;
            flt.pSkip = g_LocalPlayer;
            g_EngineTrace->TraceRay(ray, MASK_SOLID_BRUSHONLY, &flt, &fag);

            if (fag.fraction != 1.f && fag.fraction != 0.f) {
                bDidJump = true;
                cmd->buttons |= IN_DUCK; // duck
                cmd->buttons &= ~IN_JUMP; // jump
                unduck = true;
            }
        }
        for (float a = 0.f; a < max_radias; a += step) {
            Vector pt;
            pt.x = ((xThick - 20.f) * cos(a)) + pos.x;
            pt.y = ((xThick - 20.f) * sin(a)) + pos.y;
            pt.z = pos.z;

            Vector pt2 = pt;
            pt2.z -= 6;

            trace_t fag;

            Ray_t ray;
            ray.Init(pt, pt2);

            CTraceFilter flt;
            flt.pSkip = g_LocalPlayer;
            g_EngineTrace->TraceRay(ray, MASK_SOLID_BRUSHONLY, &flt, &fag);

            if (fag.fraction != 1.f && fag.fraction != 0.f) {
                bDidJump = true;
                cmd->buttons |= IN_DUCK; // duck
                cmd->buttons &= ~IN_JUMP; // jump
                unduck = true;
            }
        }
    }
}