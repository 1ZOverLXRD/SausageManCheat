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
    float MaxSpeed = 80.0f;
    float MinSpeed = 3.0f;
    float RampDist = 80.0f;
    float DeadZone = 3.0f;
    int  AimBone = 2;          // 默认 Head
    bool TeamCheck = true;
    bool VisibleOnly = true;
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
            ImGui::SliderInt("激活键", &Aimbot::Key, 0, 255, "VK_%d");
            ImGui::SliderFloat("最大速度", &Aimbot::MaxSpeed, 1.0f, 300.0f);
            ImGui::SliderFloat("最小速度", &Aimbot::MinSpeed, 0.5f, 50.0f);
            ImGui::SliderFloat("减速距离", &Aimbot::RampDist, 10.0f, 500.0f);
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

    ImGui::End();
}

void Save() {
    // 简单 ini 写入（后续可扩展）
    FILE* f = fopen("SausageMan_Config.ini", "w");
    if (!f) return;
    fprintf(f, "[ESP]\n");
    fprintf(f, "Enabled=%d\n", ESP::Enabled ? 1 : 0);
    fprintf(f, "Box2D=%d\n", ESP::Box2D ? 1 : 0);
    fprintf(f, "Box3D=%d\n", ESP::Box3D ? 1 : 0);
    fprintf(f, "Skeleton=%d\n", ESP::Skeleton ? 1 : 0);
    fprintf(f, "HealthBar=%d\n", ESP::HealthBar ? 1 : 0);
    fprintf(f, "Name=%d\n", ESP::Name ? 1 : 0);
    fprintf(f, "Distance=%d\n", ESP::Distance ? 1 : 0);
    fprintf(f, "MaxDist=%.0f\n", ESP::MaxDist);
    fprintf(f, "TeamCheck=%d\n", ESP::TeamCheck ? 1 : 0);
    fprintf(f, "\n[Aimbot]\n");
    fprintf(f, "Enabled=%d\n", Aimbot::Enabled ? 1 : 0);
    fprintf(f, "Key=%d\n", Aimbot::Key);
    fprintf(f, "MaxSpeed=%.1f\n", Aimbot::MaxSpeed);
    fprintf(f, "MinSpeed=%.1f\n", Aimbot::MinSpeed);
    fprintf(f, "RampDist=%.1f\n", Aimbot::RampDist);
    fprintf(f, "DeadZone=%.1f\n", Aimbot::DeadZone);
    fprintf(f, "AimBone=%d\n", Aimbot::AimBone);
    fprintf(f, "TeamCheck=%d\n", Aimbot::TeamCheck ? 1 : 0);
    fclose(f);
    Log::Printf("[Config] 配置已保存");
}

void Load() {
    // 简单 ini 读取
    FILE* f = fopen("SausageMan_Config.ini", "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        int v;
        float fv;
        if (sscanf(line, "Enabled=%d", &v) == 1) { ESP::Enabled = v != 0; }
        else if (sscanf(line, "Box2D=%d", &v) == 1) { ESP::Box2D = v != 0; }
        else if (sscanf(line, "Box3D=%d", &v) == 1) { ESP::Box3D = v != 0; }
        else if (sscanf(line, "Skeleton=%d", &v) == 1) { ESP::Skeleton = v != 0; }
        else if (sscanf(line, "HealthBar=%d", &v) == 1) { ESP::HealthBar = v != 0; }
        else if (sscanf(line, "Name=%d", &v) == 1) { ESP::Name = v != 0; }
        else if (sscanf(line, "Distance=%d", &v) == 1) { ESP::Distance = v != 0; }
        else if (sscanf(line, "MaxDist=%f", &fv) == 1) { ESP::MaxDist = fv; }
        else if (sscanf(line, "TeamCheck=%d", &v) == 1) { ESP::TeamCheck = v != 0; }
        else if (sscanf(line, "MaxSpeed=%f", &fv) == 1) { Aimbot::MaxSpeed = fv; }
        else if (sscanf(line, "MinSpeed=%f", &fv) == 1) { Aimbot::MinSpeed = fv; }
        else if (sscanf(line, "RampDist=%f", &fv) == 1) { Aimbot::RampDist = fv; }
        else if (sscanf(line, "DeadZone=%f", &fv) == 1) { Aimbot::DeadZone = fv; }
        else if (sscanf(line, "AimBone=%d", &v) == 1) { Aimbot::AimBone = v; }
    }
    fclose(f);
}

} // namespace Config