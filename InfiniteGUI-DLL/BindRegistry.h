#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cctype>

// 可绑定动作：供 .bind 聊天命令查询 / 修改 / 触发
struct BindableAction
{
    std::string name;                    // 命令中使用的名称（小写，如 "fakeblock"）
    std::string label;                   // 显示名称
    std::function<int()> getKey;         // 获取当前绑定键（0 = 未绑定）
    std::function<void(int)> setKey;     // 设置绑定键
    std::function<void()> onActivate;    // 按下绑定键时触发（可为空：模块自己轮询按键）
};

// 绑定注册表：模块在构造时注册自己的可绑定动作
class BindRegistry
{
public:
    static BindRegistry& Instance()
    {
        static BindRegistry registry;
        return registry;
    }

    void Register(const BindableAction& action)
    {
        for (auto& existing : actions)
        {
            if (existing.name == action.name)
            {
                existing = action; // 已存在则覆盖（Reset 后重新注册）
                return;
            }
        }
        actions.push_back(action);
    }

    BindableAction* Find(const std::string& name)
    {
        std::string lower = ToLower(name);
        for (auto& action : actions)
        {
            if (action.name == lower)
                return &action;
        }
        return nullptr;
    }

    // 按键派发：游戏内按下某键时调用，命中绑定键则触发 onActivate
    bool Dispatch(int vk)
    {
        if (vk <= 0)
            return false;
        bool hit = false;
        for (auto& action : actions)
        {
            if (action.getKey && action.getKey() == vk)
            {
                if (action.onActivate)
                    action.onActivate();
                hit = true;
            }
        }
        return hit;
    }

    const std::vector<BindableAction>& Actions() const
    {
        return actions;
    }

private:
    static std::string ToLower(std::string s)
    {
        for (auto& c : s)
            c = (char)std::tolower((unsigned char)c);
        return s;
    }

    std::vector<BindableAction> actions;
};
