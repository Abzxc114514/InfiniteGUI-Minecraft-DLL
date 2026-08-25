#pragma once
#include <chrono>
#include "Item.h"
#include "RenderModule.h"
#include "UpdateModule.h"
#include "KeybindModule.h"
#include "KeyState.h"

// 假防砍（纯本地装饰）：
//   按住右键时，在本地渲染 1.7.10 风格的防砍动画（剑影 + 格挡弧光）。
//   只画 ImGui 覆盖层动画，不读取/修改游戏内存，不注入任何输入，不向服务器发送任何数据。
class FakeBlock : public Item, public RenderModule, public UpdateModule, public KeybindModule
{
public:
    FakeBlock()
    {
        type = Visual; // 信息项类型
        name = u8"假防砍";
        description = u8"按住右键时本地渲染 1.7.10 风格防砍动画（纯本地装饰，不向服务器发送任何数据）";
        icon = "f";
        updateIntervalMs = 5;
        lastUpdateTime = std::chrono::steady_clock::now();
        FakeBlock::Reset();
        RegisterBind();
    }
    ~FakeBlock() override = default;

    static FakeBlock& Instance()
    {
        static FakeBlock instance;
        return instance;
    }

    void Toggle() override;
    void Reset() override
    {
        ResetKeybind();
        isEnabled = false;

        keybinds.insert(std::make_pair(u8"切换键：", 'B'));

        progress = 0.0f;
        animDurationMs = 150.0f;
        opacity = 0.9f;
        sizeScale = 1.0f;
        showSword = true;
        showArc = true;
        swordColor = ImVec4(0.85f, 0.88f, 0.95f, 1.0f);
        arcColor = ImVec4(0.35f, 0.65f, 1.0f, 1.0f);
        lastFrameTime = std::chrono::steady_clock::now();

        dirtyState.contentDirty = true;
        dirtyState.animating = true;
    }

    void Update() override;
    void RenderGui() override;
    void RenderBeforeGui() override {}
    void RenderAfterGui() override {}
    // 切换键统一由 ChatCommand 的绑定派发处理（模块关闭时也能重新开启）
    void OnKeyEvent(bool state, bool isRepeat, WPARAM key) override
    {
        (void)state; (void)isRepeat; (void)key;
    }

    void Load(const nlohmann::json& j) override;
    void Save(nlohmann::json& j) const override;
    void DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth) override;

private:
    void RegisterBind();
    void DrawSword(ImDrawList* drawList, const ImVec2& pivot, float scale, float angleDeg, float alpha) const;

    float progress = 0.0f;        // 0=收起 1=完全格挡
    float animDurationMs = 150.0f;
    float opacity = 0.9f;
    float sizeScale = 1.0f;
    bool showSword = true;
    bool showArc = true;
    ImVec4 swordColor;
    ImVec4 arcColor;
    std::chrono::steady_clock::time_point lastFrameTime;
};
