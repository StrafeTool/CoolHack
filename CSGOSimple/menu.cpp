#include "Menu.hpp"
#define NOMINMAX
#include <Windows.h>
#include <chrono>

#include "valve_sdk/csgostructs.hpp"
#include "helpers/input.hpp"
#include "options.hpp"
#include "ui.hpp"
#include "config.hpp"
#include "menu_background.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"
#include "imgui/impl/imgui_impl_dx9.h"
#include "imgui/impl/imgui_impl_win32.h"

/* Menu Background */
#include <d3dx9.h>
#pragma comment(lib, "D3dx9")

namespace ImGuiEx
{
    inline bool ColorEdit4(const char* label, Color* v, ImGuiColorEditFlags flags)
    {
        auto clr = ImVec4{
            v->r() / 255.0f,
            v->g() / 255.0f,
            v->b() / 255.0f,
            v->a() / 255.0f
        };
        if (ImGui::ColorEdit4(label, &clr.x, flags)) {
            v->SetColor(clr.x, clr.y, clr.z, clr.w);
            return true;
        }
        return false;
    }
}

template<size_t N>
void render_tabs(char* (&names)[N], int& activetab, float w, float h, bool sameline)
{
    bool values[N] = { false };

    values[activetab] = true;

    for(auto i = 0; i < N; ++i) {
        if(ImGui::ToggleButton(names[i], &values[i], ImVec2{ w, h })) {
            activetab = i;
        }
        if(sameline && i < N - 1)
            ImGui::SameLine();
    }
}

void Menu::Initialize()
{
	CreateStyle();  
    _visible = true;
}

void Menu::Shutdown()
{
    ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Menu::OnDeviceLost()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
}

void Menu::OnDeviceReset()
{
    ImGui_ImplDX9_CreateDeviceObjects();
}

void Menu::Render()
{
	ImGui::GetIO().MouseDrawCursor = _visible;

    if(!_visible)
        return;

    int width = 500;
    int height = 400;

    /* Super Cool Image In Menu
    * 
    IDirect3DTexture9* TextureImage = nullptr;
    if (TextureImage == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_D3DDevice9, &menu_background, sizeof(menu_background), width, height, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, NULL, NULL, &TextureImage);
    ImGui::Image(TextureImage, ImVec2(width, height));
    * 
    */

    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiSetCond_Once);
	if (ImGui::Begin("##Clean", &_visible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar)) {        
        ImGui::BeginGroupBox("##BeginGroupBox"); {
            ImGui::Checkbox("Fullbright", g_Options.misc_fullbright);
            ImGui::SliderFloat("Darkness", g_Options.misc_nightmode, 0.f, 1.f);
            ImGui::SliderFloat("Aspect Ratio", g_Options.misc_aspect_ratio, 0.f, 10.f);
            ImGui::SliderInt("Viewmodel", g_Options.viewmodel_fov, 68, 120);
            ImGui::SliderInt("Offset X", g_Options.viewmodel_offset_x, -20, 20);
            ImGui::SliderInt("Offset Y", g_Options.viewmodel_offset_y, -20, 20);
            ImGui::SliderInt("Offset Z", g_Options.viewmodel_offset_z, -20, 20);
            ImGui::Checkbox("Backtrack", g_Options.misc_backtrack);
            ImGui::Checkbox("Desync", g_Options.misc_desync);
            ImGui::Checkbox("Silent Walk", g_Options.misc_silentwalk);
            ImGui::Checkbox("Bomb timer", g_Options.misc_bomb_timer);
            ImGui::Checkbox("RCS", g_Options.misc_rcs);
            ImGui::Checkbox("Hitmarker", g_Options.misc_hitmarker);
            ImGui::Checkbox("Bhop", g_Options.misc_bhop);
            ImGui::Checkbox("Localplayer Info", g_Options.misc_info);
            ImGui::Checkbox("Jumpbug", g_Options.misc_jumpbug); ImGui::SameLine();
            ImGui::Hotkey("##JumpBugHotKey", g_Options.misc_jumpbug_hotkey, ImVec2(60, 20));
            ImGui::Checkbox("Remove scope", g_Options.misc_remove_scope);
            ImGui::Checkbox("Thirdperson", g_Options.misc_thirdperson); ImGui::SameLine();
            ImGui::Hotkey("##ThirdpersonHotkey", g_Options.misc_thirdperson_hotkey, ImVec2(60, 20));
            ImGui::Checkbox("Spectators", g_Options.misc_spectators);
            ImGui::Checkbox("Weapon ESP", g_Options.esp_player_weapons);
            ImGui::Checkbox("Ammo ESP", g_Options.esp_player_ammo);
            ImGui::Checkbox("Health ESP", g_Options.esp_player_health);
            ImGui::Checkbox("Name ESP", g_Options.esp_player_names);
            ImGui::Checkbox("Chams", g_Options.chams); ImGui::SameLine();
            ImGuiEx::ColorEdit4("##ChamsVisible", g_Options.color_chams_visible, ImGuiColorEditFlags_NoInputs);
            ImGui::Checkbox("Chams XYZ", g_Options.chams_xyz); ImGui::SameLine();
            ImGuiEx::ColorEdit4("##ChamsOccluded", g_Options.color_chams_occluded, ImGuiColorEditFlags_NoInputs);
            ImGui::Checkbox("Glow", g_Options.glow); ImGui::SameLine();
            ImGuiEx::ColorEdit4("##Glow", g_Options.color_glow, ImGuiColorEditFlags_NoInputs);
            ImGui::Checkbox("Enemy Only", g_Options.enemies_only);
            if (ImGui::Button("Save cfg", ImVec2(120, 20))) {
                Config::Get().Save();
            } ImGui::SameLine();
            if (ImGui::Button("Load cfg", ImVec2(120, 20))) {
                Config::Get().Load();
            }
        } ImGui::EndGroupBox();
        ImGui::End();
    }
}

void Menu::Toggle()
{
    _visible = !_visible;
}

void Menu::CreateStyle()
{
	ImGui::StyleColorsDark();
    ImGui::SetColorEditOptions(ImGuiColorEditFlags_HEX);
    _style.FrameRounding = 1.f;
    _style.WindowRounding = 1.f;
    _style.ChildRounding = 1.f;
    ImGui::GetStyle() = _style;
}

