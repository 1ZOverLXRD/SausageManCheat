#include "Cheat/ESP.h"
#include "Cheat/Config.h"
#include "SDK/Game.h"
#include "SDK/PlayerManager.h"
#include "Core/Log.h"
#include <cmath>
#include <cstdio>
#include <string>

namespace ESP {

static ImDrawList* s_draw = nullptr;

ImDrawList* GetDrawList() { return s_draw; }

// ---- 颜色工具 ----
static ImU32 Color(float r, float g, float b, float a = 255.0f) {
    return IM_COL32((int)r, (int)g, (int)b, (int)a);
}

// 血量颜色（绿→黄→红）
static ImU32 HealthColor(float ratio) {
    ratio = ratio < 0 ? 0 : (ratio > 1 ? 1 : ratio);
    if (ratio > 0.5f) {
        float t = (ratio - 0.5f) * 2.0f;  // 0=黄 1=绿
        return Color(255 * (1 - t) + 0 * t, 255, 0);
    } else {
        float t = ratio * 2.0f;  // 0=红 1=黄
        return Color(255, 255 * t, 0);
    }
}

// 队伍颜色（本地绿色，敌方红色）
static ImU32 TeamColor(bool isEnemy) {
    return isEnemy ? Color(255, 60, 60) : Color(60, 255, 60);
}

// ---- 绘制 2D Box ----
static void DrawBox2D(const PlayerInfo& p, ImU32 col) {
    // 用骨骼高度自适应，否则默认
    float boxHeight = 70.0f, boxWidth = 30.0f, centerY = p.screenY;

    if (p.boneValid[BONE_HEAD] && (p.boneValid[BONE_LEFT_FOOT] || p.boneValid[BONE_RIGHT_FOOT])) {
        float footY = p.boneValid[BONE_LEFT_FOOT] ? p.boneScreen[BONE_LEFT_FOOT][1]
                                                   : p.boneScreen[BONE_RIGHT_FOOT][1];
        boxHeight = std::abs(p.boneScreen[BONE_HEAD][1] - footY) * 1.2f;
        boxWidth = boxHeight * 0.35f;
        centerY = (p.boneScreen[BONE_HEAD][1] + footY) * 0.5f;
    } else if (p.boneValid[BONE_HIP] && p.boneValid[BONE_HEAD]) {
        boxHeight = std::abs(p.boneScreen[BONE_HEAD][1] - p.boneScreen[BONE_HIP][1]) * 1.5f;
        boxWidth = boxHeight * 0.35f;
    }

    ImVec2 min(p.screenX - boxWidth, centerY - boxHeight * 0.5f);
    ImVec2 max(p.screenX + boxWidth, centerY + boxHeight * 0.5f);
    s_draw->AddRect(min, max, col, 0.0f, 0, 1.5f);
}

// ---- 绘制 3D Box ----
static void DrawBox3D(const PlayerInfo& p, ImU32 col) {
    // 计算 8 个顶点（朝向相机）
    // 需要 世界尺寸（高度/宽度），从骨骼计算
    float heightWorld = 1.8f, widthWorld = 0.6f, depthWorld = 0.4f;
    float centerY = p.pos[1];

    if (p.boneValid[BONE_HEAD] && (p.boneValid[BONE_LEFT_FOOT] || p.boneValid[BONE_RIGHT_FOOT])) {
        float footY = p.boneValid[BONE_LEFT_FOOT] ? p.bonePos[BONE_LEFT_FOOT][1]
                                                    : p.bonePos[BONE_RIGHT_FOOT][1];
        heightWorld = std::abs(p.bonePos[BONE_HEAD][1] - footY) * 1.15f;
        centerY = footY + heightWorld * 0.5f;
    }
    if (p.boneValid[BONE_LEFT_HAND] && p.boneValid[BONE_RIGHT_HAND]) {
        float dx = p.bonePos[BONE_LEFT_HAND][0] - p.bonePos[BONE_RIGHT_HAND][0];
        float dy = p.bonePos[BONE_LEFT_HAND][1] - p.bonePos[BONE_RIGHT_HAND][1];
        float dz = p.bonePos[BONE_LEFT_HAND][2] - p.bonePos[BONE_RIGHT_HAND][2];
        float armSpan = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (armSpan > 0.3f) {
            widthWorld = armSpan * 0.85f;
            depthWorld = widthWorld * 0.6f;
        }
    }

    // 朝向相机
    float* camPos = Game::GetCamPos();
    float cx = camPos[0] - p.pos[0], cy = camPos[1] - centerY, cz = camPos[2] - p.pos[2];
    float len = std::sqrt(cx*cx + cy*cy + cz*cz);
    if (len < 0.001f) return;
    cx /= len; cy /= len; cz /= len;

    // 生成垂直向量（世界 Y 轴）
    float ux = 0, uy = 1, uz = 0;
    // 右向量 = forward × up
    float rx = cy*uz - cz*uy;
    float ry = cz*ux - cx*uz;
    float rz = cx*uy - cy*ux;
    // 归一化
    float rlen = std::sqrt(rx*rx + ry*ry + rz*rz);
    if (rlen < 0.001f) return;
    rx /= rlen; ry /= rlen; rz /= rlen;

    // 前向量 = forward（已归一化）
    // 深度方向也朝 forward（简化为正交盒）

    // 8 个顶点：底 4 + 顶 4
    float hw = widthWorld * 0.5f, hd = depthWorld * 0.5f, hh = heightWorld * 0.5f;

    // 底 4 点（y = centerY - hh）
    // 顶 4 点（y = centerY + hh）
    // 用 right 向量偏移宽度，forward 向量偏移深度
    float corners[8][3];
    for (int i = 0; i < 4; i++) {
        float sx = (i & 1) ? hw : -hw;
        float sz = (i & 2) ? hd : -hd;
        corners[i][0] = p.pos[0] + rx*sx + cx*sz;
        corners[i][1] = centerY - hh;
        corners[i][2] = p.pos[2] + rz*sx + cz*sz;
        corners[i+4][0] = corners[i][0];
        corners[i+4][1] = centerY + hh;
        corners[i+4][2] = corners[i][2];
    }

    // W2S 投影
    float scr[8][2];
    bool valid[8] = {false};
    for (int i = 0; i < 8; i++) {
        valid[i] = CameraManager::WorldToScreen(corners[i][0], corners[i][1], corners[i][2], scr[i][0], scr[i][1]);
    }
    if (!valid[0] || !valid[1] || !valid[2] || !valid[3] ||
        !valid[4] || !valid[5] || !valid[6] || !valid[7]) return;

    // 12 条边
    // 底面: 0-1, 1-3, 3-2, 2-0
    s_draw->AddLine(ImVec2(scr[0][0], scr[0][1]), ImVec2(scr[1][0], scr[1][1]), col, 1.5f);
    s_draw->AddLine(ImVec2(scr[1][0], scr[1][1]), ImVec2(scr[3][0], scr[3][1]), col, 1.5f);
    s_draw->AddLine(ImVec2(scr[3][0], scr[3][1]), ImVec2(scr[2][0], scr[2][1]), col, 1.5f);
    s_draw->AddLine(ImVec2(scr[2][0], scr[2][1]), ImVec2(scr[0][0], scr[0][1]), col, 1.5f);
    // 顶面: 4-5, 5-7, 7-6, 6-4
    s_draw->AddLine(ImVec2(scr[4][0], scr[4][1]), ImVec2(scr[5][0], scr[5][1]), col, 1.5f);
    s_draw->AddLine(ImVec2(scr[5][0], scr[5][1]), ImVec2(scr[7][0], scr[7][1]), col, 1.5f);
    s_draw->AddLine(ImVec2(scr[7][0], scr[7][1]), ImVec2(scr[6][0], scr[6][1]), col, 1.5f);
    s_draw->AddLine(ImVec2(scr[6][0], scr[6][1]), ImVec2(scr[4][0], scr[4][1]), col, 1.5f);
    // 竖边: 0-4, 1-5, 2-6, 3-7
    for (int i = 0; i < 4; i++) {
        s_draw->AddLine(ImVec2(scr[i][0], scr[i][1]), ImVec2(scr[i+4][0], scr[i+4][1]), col, 1.5f);
    }
}

// ---- 绘制骨骼 ----
static void DrawSkeleton(const PlayerInfo& p, ImU32 col) {
    auto bone = [&](int b) -> bool {
        return p.boneValid[b];
    };
    auto BP = [&](int b) -> ImVec2 {
        return ImVec2(p.boneScreen[b][0], p.boneScreen[b][1]);
    };

    // 连接关系
    // Head → Spine → Hip → LeftFoot
    //          Spine → Hip → RightFoot
    // 头部 → 手
    if (bone(BONE_HEAD) && bone(BONE_SPINE))
        s_draw->AddLine(BP(BONE_HEAD), BP(BONE_SPINE), col, 1.5f);
    if (bone(BONE_SPINE) && bone(BONE_HIP))
        s_draw->AddLine(BP(BONE_SPINE), BP(BONE_HIP), col, 1.5f);
    if (bone(BONE_HIP) && bone(BONE_LEFT_FOOT))
        s_draw->AddLine(BP(BONE_HIP), BP(BONE_LEFT_FOOT), col, 1.5f);
    if (bone(BONE_HIP) && bone(BONE_RIGHT_FOOT))
        s_draw->AddLine(BP(BONE_HIP), BP(BONE_RIGHT_FOOT), col, 1.5f);
    if (bone(BONE_HEAD) && bone(BONE_LEFT_HAND))
        s_draw->AddLine(BP(BONE_HEAD), BP(BONE_LEFT_HAND), col, 1.5f);
    if (bone(BONE_HEAD) && bone(BONE_RIGHT_HAND))
        s_draw->AddLine(BP(BONE_HEAD), BP(BONE_RIGHT_HAND), col, 1.5f);
    // 身体 → 手（通过 Spine 或 SkinBody）
    if (bone(BONE_SKIN_BODY) && bone(BONE_LEFT_HAND))
        s_draw->AddLine(BP(BONE_SKIN_BODY), BP(BONE_LEFT_HAND), col, 1.5f);
    if (bone(BONE_SKIN_BODY) && bone(BONE_RIGHT_HAND))
        s_draw->AddLine(BP(BONE_SKIN_BODY), BP(BONE_RIGHT_HAND), col, 1.5f);
}

// ---- 血量条 ----
static void DrawHealthBar(const PlayerInfo& p, float x, float y, float h) {
    if (p.maxHp <= 0) return;
    float ratio = p.hp / p.maxHp;
    if (ratio > 1) ratio = 1;
    if (ratio < 0) ratio = 0;

    const float barW = 4.0f;
    ImU32 col = HealthColor(ratio);

    // 背景
    s_draw->AddRectFilled(ImVec2(x - 1, y - 1), ImVec2(x + barW + 1, y + h + 1), Color(0, 0, 0, 180));
    // 血量
    float fillH = h * ratio;
    s_draw->AddRectFilled(ImVec2(x, y + h - fillH), ImVec2(x + barW, y + h), col);
    // 边框
    s_draw->AddRect(ImVec2(x, y), ImVec2(x + barW, y + h), Color(255, 255, 255, 100));
}

// ---- 主绘制 ----
void Draw() {
    s_draw = ImGui::GetBackgroundDrawList();

    auto& players = Game::GetPlayers();
    if (players.empty()) return;

    // 本地队伍（用于同队过滤）
    int64_t localTeam = 0;
    for (auto& p : players) {
        if (p.isLocal) { localTeam = p.team; break; }
    }

    int sw, sh;
    CameraManager::GetScreenSize(sw, sh);

    for (auto& p : players) {
        // 过滤
        if (!p.valid || !p.alive) continue;
        if (!Config::ESP::IsLocal && p.isLocal) continue;          // 默认不显示本地
        if (Config::ESP::TeamCheck && localTeam != 0 && p.team != 0 && p.team == localTeam && !p.isLocal) continue;  // 同队
        if (!p.onScreen) continue;
        if (p.distToCam > Config::ESP::MaxDist) continue;

        bool isEnemy = p.team != localTeam;
        ImU32 col = TeamColor(isEnemy);

        // Box
        if (Config::ESP::Box2D) DrawBox2D(p, col);
        if (Config::ESP::Box3D && !Config::ESP::Box2D) DrawBox3D(p, col);

        // 骨骼
        if (Config::ESP::Skeleton) DrawSkeleton(p, col);

        // 名字 + 距离
        std::string text;
        if (Config::ESP::Name && p.name[0]) text += p.name;
        if (Config::ESP::Distance) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.0fm", p.distToCam);
            if (!text.empty()) text += " ";
            text += buf;
        }
        if (!text.empty()) {
            ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
            s_draw->AddText(ImVec2(p.screenX - textSize.x * 0.5f, p.screenY - 60), col, text.c_str(), text.c_str() + text.size());
        }

        // 血量条（在 Box 左侧）
        if (Config::ESP::HealthBar) {
            // Box 高度
            float boxH = 70.0f;
            if (p.boneValid[BONE_HEAD] && (p.boneValid[BONE_LEFT_FOOT] || p.boneValid[BONE_RIGHT_FOOT])) {
                float footY = p.boneValid[BONE_LEFT_FOOT] ? p.boneScreen[BONE_LEFT_FOOT][1]
                                                           : p.boneScreen[BONE_RIGHT_FOOT][1];
                boxH = std::abs(p.boneScreen[BONE_HEAD][1] - footY) * 1.2f;
            }
            DrawHealthBar(p, p.screenX - 16, p.screenY - boxH * 0.5f, boxH);
        }
    }
}

} // namespace ESP