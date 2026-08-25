#pragma once
#include <string>
#include "Item.h"
#include "KeybindModule.h"

// 聊天栏命令模块：
//   在 Minecraft 聊天栏中输入  .bind <模块名> <按键>  即可为模块绑定按键
//   .bind list            查看所有可绑定模块
//   .unbind <模块名>      解除绑定
// 纯本地实现：只截获键盘消息做解析，不读取/修改游戏内存。
class ChatCommand : public Item, public KeybindModule
{
public:
    ChatCommand()
    {
        type = Hidden; // 不在模块面板显示，但始终运行
        name = u8"聊天命令";
        description = u8"在游戏聊天栏输入 .bind <模块> <按键> 绑定按键（纯本地）";
        icon = "c";
        isEnabled = true;
    }
    ~ChatCommand() override = default;

    static ChatCommand& Instance()
    {
        static ChatCommand instance;
        return instance;
    }

    void Toggle() override {}
    void Reset() override
    {
        isEnabled = true; // 始终启用
        buffer.clear();
        chatOpen = false;
    }
    void OnKeyEvent(bool state, bool isRepeat, WPARAM key) override;

    void Load(const nlohmann::json& j) override
    {
        (void)j;
        isEnabled = true;
    }
    void Save(nlohmann::json& j) const override
    {
        SaveItem(j);
    }
    void DrawSettings(const float& bigPadding, const float& centerX, const float& itemWidth) override
    {
        (void)bigPadding; (void)centerX; (void)itemWidth;
    }

private:
    void ExecuteBuffer();

    std::string buffer;   // 当前输入的命令行（以 '.' 开头才记录）
    bool chatOpen = false;// 猜测聊天栏是否打开（聊天键按下 → 回车/Esc 关闭）
};
