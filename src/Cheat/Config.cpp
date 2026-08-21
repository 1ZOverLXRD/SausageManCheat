#include "Cheat/Config.h"
#include "Core/Log.h"
#include <imgui.h>
#include <cstdio>

// 外部函数声明（在 Config 命名空间外，避免 Config:: 前缀）
extern void GameDumpState();
extern void RequestUnload();

namespace Config {

bool MenuOpen = false;

namespace ESP {
    bool Enabled = true;
    bool Box2D = true;
    bool Box3D = false;
    bool Skeleton = true;
    bool HealthBar = true;
    bool Name = true;
    bool Distance = true;
    float MaxDist = 300.0f;
    bool TeamCheck = true;
    bool IsLocal = false;
}

namespace Aimbot {
    bool Enabled = false;
    int  Key = VK_RBUTTON;    // 右键激活
    float Smooth = 8.0f;      // 平滑系数 (越大越慢)
    float DeadZone = 3.0f;
    int  AimBone = 2;          // 默认 Head
    bool TeamCheck = true;
    bool VisibleOnly = true;
    float FovRadius = 150.0f;  // FOV 圆圈半径 (px)
    int   TargetMode = 0;      // 0=最近距离, 1=最中心, 2=最低血量
}

void ToggleMenu() {
    MenuOpen = !MenuOpen;
    Log::Printf("[Config] 菜单 %s", MenuOpen ? "打开" : "关闭");
}

void DrawMenu() {
    ImGui::SetNextWindowSize(ImVec2(480, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("SausageMan Cheat", &MenuOpen, ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTabBar("MainTabs")) {

        // ---- ESP Tab ----
        if (ImGui::BeginTabItem("ESP")) {
            ImGui::Checkbox("启用 ESP", &ESP::Enabled);
            ImGui::Separator();
            ImGui::Checkbox("2D Box", &ESP::Box2D);
            ImGui::Checkbox("3D Box", &ESP::Box3D);
            ImGui::Checkbox("骨骼", &ESP::Skeleton);
            ImGui::Checkbox("血量条", &ESP::HealthBar);
            ImGui::Checkbox("名字", &ESP::Name);
            ImGui::Checkbox("距离", &ESP::Distance);
            ImGui::SliderFloat("最大距离", &ESP::MaxDist, 10.0f, 1000.0f, "%.0f m");
            ImGui::Checkbox("队伍过滤", &ESP::TeamCheck);
            ImGui::Checkbox("显示本地", &ESP::IsLocal);
            ImGui::EndTabItem();
        }

        // ---- Aimbot Tab ----
        if (ImGui::BeginTabItem("Aimbot")) {
            ImGui::Checkbox("启用 Aimbot", &Aimbot::Enabled);
            ImGui::Separator();

            // 热键选择
            const char* keys[] = {"右键", "左键", "X键", "C键", "V键", "鼠标侧键上", "鼠标侧键下", "Shift", "Ctrl", "Alt"};
            int keyVals[] = {VK_RBUTTON, VK_LBUTTON, 0x58, 0x43, 0x56, VK_XBUTTON1, VK_XBUTTON2, VK_SHIFT, VK_CONTROL, VK_MENU};
            int keyIdx = 0;
            for (int i = 0; i < 10; i++) {
                if (Aimbot::Key == keyVals[i]) { keyIdx = i; break; }
            }
            if (ImGui::Combo("激活键", &keyIdx, keys, 10)) {
                Aimbot::Key = keyVals[keyIdx];
            }
            ImGui::Separator();

            // 选敌方式
            const char* modes[] = {"最近距离", "最中心", "最低血量"};
            ImGui::Combo("选敌方式", &Aimbot::TargetMode, modes, 3);
            ImGui::SliderFloat("FOV 半径", &Aimbot::FovRadius, 20.0f, 500.0f, "%.0f px");
            ImGui::Separator();

            // 瞄准参数
            ImGui::SliderFloat("平滑系数", &Aimbot::Smooth, 1.0f, 30.0f, "%.1f");
            ImGui::SliderFloat("死区", &Aimbot::DeadZone, 1.0f, 30.0f);
            const char* bones[] = {"左手", "右手", "头", "髋", "身体", "右脚", "左脚", "脊柱"};
            ImGui::Combo("瞄准骨骼", &Aimbot::AimBone, bones, 8);
            ImGui::Checkbox("队伍过滤", &Aimbot::TeamCheck);
            ImGui::Checkbox("仅屏幕内", &Aimbot::VisibleOnly);
            ImGui::EndTabItem();
        }

        // ---- 状态 Tab ----
        if (ImGui::BeginTabItem("状态")) {
            if (ImGui::Button("Dump 玩家到日志")) {
                GameDumpState();
            }
            if (ImGui::Button("卸载")) {
                RequestUnload();
            }
            ImGui::EndTabItem();
        }
    }
    ImGui::EndTabBar();  // 必须有，否则 Missing EndTabBar()

    ImGui::End();
}

void Save() {
    FILE* f = fopen("SausageMan_Config.ini", "w");
    if (!f) return;
    fprintf(f, "[ESP]\n");
    fprintf(f, "ESP.Enabled=%d\n", ESP::Enabled ? 1 : 0);
    fprintf(f, "ESP.Box2D=%d\n", ESP::Box2D ? 1 : 0);
    fprintf(f, "ESP.Box3D=%d\n", ESP::Box3D ? 1 : 0);
    fprintf(f, "ESP.Skeleton=%d\n", ESP::Skeleton ? 1 : 0);
    fprintf(f, "ESP.HealthBar=%d\n", ESP::HealthBar ? 1 : 0);
    fprintf(f, "ESP.Name=%d\n", ESP::Name ? 1 : 0);
    fprintf(f, "ESP.Distance=%d\n", ESP::Distance ? 1 : 0);
    fprintf(f, "ESP.MaxDist=%.0f\n", ESP::MaxDist);
    fprintf(f, "ESP.TeamCheck=%d\n", ESP::TeamCheck ? 1 : 0);
    fprintf(f, "ESP.IsLocal=%d\n", ESP::IsLocal ? 1 : 0);
    fprintf(f, "\n[Aimbot]\n");
    fprintf(f, "AIM.Enabled=%d\n", Aimbot::Enabled ? 1 : 0);
    fprintf(f, "AIM.Key=%d\n", Aimbot::Key);
    fprintf(f, "AIM.Smooth=%.1f\n", Aimbot::Smooth);
    fprintf(f, "AIM.DeadZone=%.1f\n", Aimbot::DeadZone);
    fprintf(f, "AIM.AimBone=%d\n", Aimbot::AimBone);
    fprintf(f, "AIM.TeamCheck=%d\n", Aimbot::TeamCheck ? 1 : 0);
    fprintf(f, "AIM.VisibleOnly=%d\n", Aimbot::VisibleOnly ? 1 : 0);
    fprintf(f, "AIM.FovRadius=%.0f\n", Aimbot::FovRadius);
    fprintf(f, "AIM.TargetMode=%d\n", Aimbot::TargetMode);
    fclose(f);
    Log::Printf("[Config] 配置已保存");
}

void Load() {
    FILE* f = fopen("SausageMan_Config.ini", "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        int v;
        float fv;
        if (sscanf(line, "ESP.Enabled=%d", &v) == 1) { ESP::Enabled = v != 0; }
        else if (sscanf(line, "ESP.Box2D=%d", &v) == 1) { ESP::Box2D = v != 0; }
        else if (sscanf(line, "ESP.Box3D=%d", &v) == 1) { ESP::Box3D = v != 0; }
        else if (sscanf(line, "ESP.Skeleton=%d", &v) == 1) { ESP::Skeleton = v != 0; }
        else if (sscanf(line, "ESP.HealthBar=%d", &v) == 1) { ESP::HealthBar = v != 0; }
        else if (sscanf(line, "ESP.Name=%d", &v) == 1) { ESP::Name = v != 0; }
        else if (sscanf(line, "ESP.Distance=%d", &v) == 1) { ESP::Distance = v != 0; }
        else if (sscanf(line, "ESP.MaxDist=%f", &fv) == 1) { ESP::MaxDist = fv; }
        else if (sscanf(line, "ESP.TeamCheck=%d", &v) == 1) { ESP::TeamCheck = v != 0; }
        else if (sscanf(line, "ESP.IsLocal=%d", &v) == 1) { ESP::IsLocal = v != 0; }
        else if (sscanf(line, "AIM.Enabled=%d", &v) == 1) { Aimbot::Enabled = v != 0; }
        else if (sscanf(line, "AIM.Key=%d", &v) == 1) { Aimbot::Key = v; }
        else if (sscanf(line, "AIM.Smooth=%f", &fv) == 1) { Aimbot::Smooth = fv; }
        else if (sscanf(line, "AIM.DeadZone=%f", &fv) == 1) { Aimbot::DeadZone = fv; }
        else if (sscanf(line, "AIM.AimBone=%d", &v) == 1) { Aimbot::AimBone = v; }
        else if (sscanf(line, "AIM.TeamCheck=%d", &v) == 1) { Aimbot::TeamCheck = v != 0; }
        else if (sscanf(line, "AIM.VisibleOnly=%d", &v) == 1) { Aimbot::VisibleOnly = v != 0; }
        else if (sscanf(line, "AIM.FovRadius=%f", &fv) == 1) { Aimbot::FovRadius = fv; }
        else if (sscanf(line, "AIM.TargetMode=%d", &v) == 1) { Aimbot::TargetMode = v; }
    }
    fclose(f);
}

} // namespace Config