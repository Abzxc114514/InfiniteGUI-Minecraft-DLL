#include "FakeBlock.h"
#include "imgui/imgui.h"
#include "ImGuiStd.h"
#include "BindRegistry.h"
#include "GameStateDetector.h"
#include "NotificationItem.h"

#include <algorithm>
#include <cmath>

void FakeBlock::Toggle()
{
    if (!isEnabled)
        progress = 0.0f; // 关闭时立刻收起
    dirtyState.animating = true;
}

void FakeBlock::RegisterBind()
{
    BindableAction action;
    action.name = "fakeblock";
    action.label = u8"假防砍";
    action.getKey = [this]() -> int
    {
        return GetBindKey(u8"切换键：");
    };
    action.setKey = [this](int vk)
    {
        SetBindKey(u8"切换键：", vk);
    };
    action.onActivate = []()
    {
        FakeBlock& fakeBlock = FakeBlock::Instance();
        fakeBlock.isEnabled = !fakeBlock.isEnabled;
        fakeBlock.Toggle();
        NotificationItem::Instance().AddNotification(NotificationType_Info,
            fakeBlock.isEnabled ? u8"假防砍：已开启" : u8"假防砍：已关闭");
    };
    BindRegistry::Instance().Register(action);
}

void FakeBlock::Update()
{
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - lastFrameTime).count();
    lastFrameTime = now;
    if (dt <= 0.0f)
        dt = 0.001f;
    if (dt > 0.25f)
        dt = 0.25f; // 掉帧/卡顿时避免动画跳变

    // 纯本地：只看右键物理状态（游戏内、窗口聚焦时）
    bool holding = false;
    if (GameStateDetector::Instance().IsInGameWindow() &&
        GameStateDetector::Instance().IsInGame())
    {
        holding = KeyState::GetKeyDown(VK_RBUTTON) != 0;
    }

    float target = holding ? 1.0f : 0.0f;
    if (animDurationMs < 20.0f)
        animDurationMs = 20.0f;
    float step = dt * 1000.0f / animDurationMs;
    if (progress < target)
        progress = std::min(target, progress + step);
    else if (progress > target)
        progress = std::max(target, progress - step);

    // 动画进行中或保持格挡时需要持续重绘
    dirtyState.animating = progress > 0.0001f;
}

static float SmoothStep01(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

void FakeBlock::RenderGui()
{
    if (progress <= 0.0001f)
        return;
    if (!GameStateDetector::Instance().IsInGame())
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    float p = SmoothStep01(progress);

    // ---- 格挡弧光：准星两侧的弧线，随格挡进度浮现 ----
    if (showArc)
    {
        float arcAlpha = arcColor.w * opacity * p;
        ImU32 arcCol = ImGui::ColorConvertFloat4ToU32(
            ImVec4(arcColor.x, arcColor.y, arcColor.z, arcAlpha));
        float radius = 34.0f * sizeScale;
        float thickness = 3.5f;

        drawList->PathArcTo(center, radius, IM_PI * 100.0f / 180.0f, IM_PI * 170.0f / 180.0f, 24);
        drawList->PathStroke(arcCol, ImDrawFlags_None, thickness);

        drawList->PathArcTo(center, radius, IM_PI * 10.0f / 180.0f, IM_PI * 80.0f / 180.0f, 24);
        drawList->PathStroke(arcCol, ImDrawFlags_None, thickness);

        // 外圈微光
        ImU32 glowCol = ImGui::ColorConvertFloat4ToU32(
            ImVec4(arcColor.x, arcColor.y, arcColor.z, arcAlpha * 0.25f));
        drawList->AddCircle(center, radius + 6.0f, glowCol, 48, 2.0f);
    }

    // ---- 剑影：从待机位（右下）平滑滑向格挡位（靠近屏幕中心） ----
    if (showSword)
    {
        ImVec2 idlePivot(center.x + io.DisplaySize.x * 0.26f,
                         center.y + io.DisplaySize.y * 0.30f);
        ImVec2 blockPivot(center.x + io.DisplaySize.x * 0.10f,
                          center.y + io.DisplaySize.y * 0.14f);

        // 格挡时的轻微呼吸摆动
        float t = std::chrono::duration<float>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        float sway = std::sin(t * 2.0f * IM_PI * 0.7f) * 2.0f * p;

        ImVec2 pivot = ImLerp(idlePivot, blockPivot, p);
        pivot.x += sway;
        pivot.y += sway * 0.5f;

        float angleDeg = ImLerp(28.0f, -18.0f, p);      // 待机右上 -> 格挡竖起
        float scale = ImLerp(0.75f, 1.10f, p) * sizeScale;
        float alpha = opacity * (0.35f + 0.65f * p);

        DrawSword(drawList, pivot, scale, angleDeg, alpha);
    }
}

void FakeBlock::DrawSword(ImDrawList* drawList, const ImVec2& pivot, float scale, float angleDeg, float alpha) const
{
    if (alpha <= 0.01f)
        return;

    float rad = angleDeg * IM_PI / 180.0f;
    float cs = std::cos(rad);
    float sn = std::sin(rad);

    // 局部坐标：剑柄在原点，剑尖朝上（-Y）
    auto Xform = [&](float x, float y) -> ImVec2
    {
        x *= scale;
        y *= scale;
        return ImVec2(pivot.x + x * cs - y * sn, pivot.y + x * sn + y * cs);
    };
    auto XformShadow = [&](float x, float y) -> ImVec2
    {
        ImVec2 v = Xform(x, y);
        return ImVec2(v.x + 3.0f, v.y + 4.0f);
    };

    ImU32 bladeCol = ImGui::ColorConvertFloat4ToU32(
        ImVec4(swordColor.x, swordColor.y, swordColor.z, swordColor.w * alpha));
    ImU32 darkCol = ImGui::ColorConvertFloat4ToU32(
        ImVec4(swordColor.x * 0.45f, swordColor.y * 0.45f, swordColor.z * 0.45f, alpha));
    ImU32 hiCol = ImGui::ColorConvertFloat4ToU32(
        ImVec4(1.0f, 1.0f, 1.0f, 0.45f * alpha));
    ImU32 shadowCol = ImGui::ColorConvertFloat4ToU32(
        ImVec4(0.0f, 0.0f, 0.0f, 0.30f * alpha));

    // 剑刃（六边形：刃身 + 剑尖）
    const ImVec2 blade[6] = {
        Xform(-6.0f, -8.0f),
        Xform(6.0f, -8.0f),
        Xform(7.0f, -70.0f),
        Xform(0.0f, -92.0f),
        Xform(-7.0f, -70.0f),
        Xform(-6.0f, -8.0f)
    };
    const ImVec2 bladeShadow[6] = {
        XformShadow(-6.0f, -8.0f),
        XformShadow(6.0f, -8.0f),
        XformShadow(7.0f, -70.0f),
        XformShadow(0.0f, -92.0f),
        XformShadow(-7.0f, -70.0f),
        XformShadow(-6.0f, -8.0f)
    };

    // 护手 / 剑柄 / 柄头
    auto DrawParts = [&](auto& xf, ImU32 col)
    {
        drawList->AddQuadFilled(xf(-17.0f, -10.0f), xf(17.0f, -10.0f),
                                xf(17.0f, -2.0f), xf(-17.0f, -2.0f), col);   // 护手
        drawList->AddQuadFilled(xf(-4.0f, -2.0f), xf(4.0f, -2.0f),
                                xf(4.0f, 26.0f), xf(-4.0f, 26.0f), col);     // 剑柄
        drawList->AddCircleFilled(xf(0.0f, 30.0f), 5.5f * scale, col, 12);    // 柄头
    };

    // 阴影层
    drawList->AddConvexPolyFilled(bladeShadow, 6, shadowCol);
    DrawParts(XformShadow, shadowCol);

    // 主体
    drawList->AddConvexPolyFilled(blade, 6, bladeCol);
    DrawParts(Xform, darkCol);

    // 刃脊高光
    drawList->AddQuadFilled(Xform(-1.2f, -70.0f), Xform(1.2f, -70.0f),
                            Xform(1.2f, -12.0f), Xform(-1.2f, -12.0f), hiCol);
}

void FakeBlock::Load(const nlohmann::json& j)
{
    LoadItem(j);
    LoadKeybind(j);
    if (j.contains("animDurationMs")) animDurationMs = j["animDurationMs"];
    if (j.contains("opacity")) opacity = j["opacity"];
    if (j.contains("sizeScale")) sizeScale = j["sizeScale"];
    if (j.contains("showSword")) showSword = j["showSword"];
    if (j.contains("showArc")) showArc = j["showArc"];
    ImGuiStd::LoadImVec4(j, "swordColor", swordColor);
    ImGuiStd::LoadImVec4(j, "arcColor", arcColor);
}

void FakeBlock::Save(nlohmann::json& j) const
{
    SaveItem(j);
    SaveKeybind(j);
    j["animDurationMs"] = animDurationMs;
    j["opacity"] = opacity;
    j["sizeScale"] = sizeScale;
    j["showSword"] = showSword;
    j["showArc"] = showArc;
    ImGuiStd::SaveImVec4(j, "swordColor", swordColor);
    ImGuiStd::SaveImVec4(j, "arcColor", arcColor);
}

void FakeBlock::DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth)
{
    float bigItemWidth = centerX * 2.0f - bigPadding * 4.0f;

    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(bigItemWidth);
    ImGui::SliderFloat(u8"动画时长(ms)", &animDurationMs, 50.0f, 400.0f, "%.0f");

    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(bigItemWidth);
    ImGui::SliderFloat(u8"不透明度", &opacity, 0.1f, 1.0f, "%.2f");

    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(bigItemWidth);
    ImGui::SliderFloat(u8"大小", &sizeScale, 0.5f, 2.0f, "%.2f");

    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::Checkbox(u8"显示剑剪影", &showSword);
    ImGui::SameLine();
    ImGui::SetCursorPosX(bigPadding + centerX);
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::Checkbox(u8"显示格挡弧光", &showArc);

    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(itemWidth);
    ImGuiStd::EditColor(u8"剑颜色", swordColor, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    ImGui::SetCursorPosX(bigPadding);
    ImGui::SetNextItemWidth(itemWidth);
    ImGuiStd::EditColor(u8"弧光颜色", arcColor, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    // 绑定键设置（也可通过聊天栏 .bind fakeblock <按键> 修改）
    DrawKeybindSettings(bigPadding, centerX, itemWidth);

    ImGui::PushFont(NULL, ImGui::GetFontSize() * 0.8f);
    ImGui::BeginDisabled();
    ImGuiStd::TextShadow(u8"说明：仅本地渲染动画，不读取/修改游戏内存，不发送任何数据。");
    ImGui::EndDisabled();
    ImGui::PopFont();
}
