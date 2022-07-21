#pragma once
#include <d3d9.h>
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

struct Feature {
//    bool aimBot = false;
    bool glowEsp = false;
    bool radarEsp = false;
    bool triggerBot = false;
    bool antiFlash = false;
    bool bHop = false;
    bool rcs = false;
    bool boxEsp = false;
    bool lineEsp = false;
};

//class ImGuiConfig;
class ImGuiConfig {
public:
    WNDCLASSEX wc;
    HWND hwnd;
    ImGuiIO io;
    ImGuiStyle style;
    bool show_demo_window;
    bool show_another_window;
    ImVec4 clear_color;
    bool done;
    Feature feature;
    ImGuiConfig();
    ~ImGuiConfig();
};
void SetCheatMenuContent(ImGuiConfig& imGuiConfig);
void HandleWindowMessage(ImGuiConfig &imGuiConfig);
void HandleMenuRendering(const ImGuiConfig &imGuiConfig);
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);