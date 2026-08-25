#include "ChatCommand.h"
#include "BindRegistry.h"
#include "GameKeyBind.h"
#include "GameStateDetector.h"
#include "Menu.h"
#include "NotificationItem.h"
#include "VK_Keymap.h"

#include <cctype>
#include <cstdio>
#include <vector>

// VK -> 字符（用于把按键还原成聊天里敲的字符）
static char VkToChar(int vk)
{
    bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    if (vk >= 'A' && vk <= 'Z')
        return (char)(shift ? vk : vk + 32);
    if (vk >= '0' && vk <= '9')
        return (char)vk;
    switch (vk)
    {
    case VK_OEM_PERIOD: return '.';
    case VK_OEM_COMMA:  return ',';
    case VK_OEM_MINUS:  return '-';
    case VK_OEM_PLUS:   return '=';
    case VK_OEM_2:      return '/';
    case VK_OEM_1:      return ';';
    case VK_OEM_7:      return '\'';
    case VK_OEM_4:      return '[';
    case VK_OEM_6:      return ']';
    case VK_OEM_5:      return '\\';
    case VK_OEM_3:      return '`';
    case VK_SPACE:      return ' ';
    default:            return 0;
    }
}

static std::string ToLowerStr(std::string s)
{
    for (auto& c : s)
        c = (char)std::tolower((unsigned char)c);
    return s;
}

static std::vector<std::string> SplitSpaces(const std::string& s)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : s)
    {
        if (c == ' ')
        {
            if (!cur.empty())
            {
                out.push_back(cur);
                cur.clear();
            }
        }
        else
        {
            cur.push_back(c);
        }
    }
    if (!cur.empty())
        out.push_back(cur);
    return out;
}

// 按键名 -> VK（"r"、"f1"、"space"、"mouse.left" 等），解析失败返回 -1
static int ParseKeyName(std::string key)
{
    key = ToLowerStr(key);
    if (key == "none" || key == "unbind" || key == "clear")
        return 0;
    if (key.size() == 1)
    {
        unsigned char c = (unsigned char)std::toupper((unsigned char)key[0]);
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
            return (int)c;
        switch (c)
        {
        case '.':  return VK_OEM_PERIOD;
        case ',':  return VK_OEM_COMMA;
        case '-':  return VK_OEM_MINUS;
        case '=':  return VK_OEM_PLUS;
        case '/':  return VK_OEM_2;
        case ';':  return VK_OEM_1;
        case '\'': return VK_OEM_7;
        case '[':  return VK_OEM_4;
        case ']':  return VK_OEM_6;
        case '\\': return VK_OEM_5;
        case '`':  return VK_OEM_3;
        default:   return -1;
        }
    }
    if (key.rfind("mouse.", 0) == 0)
        key = key.substr(6);
    auto it = STRING_TO_VK.find(key);
    if (it != STRING_TO_VK.end())
        return it->second;
    auto mouseIt = STRING_Mouse_TO_VK.find(key);
    if (mouseIt != STRING_Mouse_TO_VK.end())
        return mouseIt->second;
    return -1;
}

static std::string KeyDisplayName(int vk)
{
    if (vk == 0)
        return u8"无绑定";
    if (vk > 0 && vk < 256)
    {
        std::string n = keys[vk];
        if (!n.empty() && n[0] != '-')
            return n;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "VK_%02X", vk);
    return buf;
}

void ChatCommand::OnKeyEvent(bool state, bool isRepeat, WPARAM key)
{
    if (!state)
        return;
    int vk = (int)key;

    // 回车 / Esc：提交或取消当前命令
    if (vk == VK_RETURN || vk == VK_ESCAPE)
    {
        if (chatOpen)
        {
            if (vk == VK_RETURN && !isRepeat)
                ExecuteBuffer();
            buffer.clear();
            chatOpen = false;
        }
        return;
    }

    // 窗口失焦 / 自家菜单打开时不处理
    if (!GameStateDetector::Instance().IsInGameWindow())
    {
        buffer.clear();
        chatOpen = false;
        return;
    }
    if (Menu::Instance().isEnabled)
    {
        buffer.clear();
        chatOpen = false;
        return;
    }

    if (chatOpen)
    {
        if (isRepeat)
            return;
        if (vk == VK_BACK)
        {
            if (!buffer.empty())
                buffer.pop_back();
            return;
        }
        if (vk == VK_TAB)
            return; // Tab 是命令补全，不记录
        char ch = VkToChar(vk);
        if (ch == 0)
            return;
        if (buffer.empty())
        {
            if (ch == '.') // 只记录 '.' 开头的内容，避免污染普通聊天
                buffer.push_back(ch);
        }
        else if (buffer.size() < 64)
        {
            buffer.push_back(ch);
        }
        return;
    }

    // 聊天栏打开检测：按下聊天键 / 命令键（从游戏内按下时光标必然隐藏，处于 InGame）
    if (GameStateDetector::Instance().IsInGame())
    {
        int chatKey = GameKeyBind::Instance().GetVK(GameAction::Chat);
        int cmdKey = GameKeyBind::Instance().GetVK(GameAction::Command);
        if ((chatKey > 0 && vk == chatKey) || (cmdKey > 0 && vk == cmdKey))
        {
            chatOpen = true;
            buffer.clear();
            return;
        }
    }

    // 普通游戏内按键 -> 派发绑定键（聊天栏打开时上面已 return，不会误触发）
    if (!isRepeat && GameStateDetector::Instance().IsInGame())
        BindRegistry::Instance().Dispatch(vk);
}

void ChatCommand::ExecuteBuffer()
{
    std::vector<std::string> tokens = SplitSpaces(buffer);
    if (tokens.empty())
        return;

    std::string cmd = ToLowerStr(tokens[0]);

    if (cmd == ".bind")
    {
        // .bind list：列出全部可绑定模块
        if (tokens.size() == 2 && ToLowerStr(tokens[1]) == "list")
        {
            std::string msg = u8"可绑定模块：";
            for (const auto& action : BindRegistry::Instance().Actions())
                msg += "\n" + action.name + " (" + KeyDisplayName(action.getKey ? action.getKey() : 0) + ")";
            NotificationItem::Instance().AddNotification(NotificationType_Info, msg, 6000);
            return;
        }
        if (tokens.size() < 3)
        {
            NotificationItem::Instance().AddNotification(NotificationType_Warning,
                u8"用法：.bind <模块名> <按键>\n例如：.bind fakeblock r\n.bind list 查看全部模块", 6000);
            return;
        }

        std::string name = ToLowerStr(tokens[1]);
        int vk = ParseKeyName(tokens[2]);
        if (vk < 0)
        {
            NotificationItem::Instance().AddNotification(NotificationType_Warning,
                u8"无法识别按键：" + tokens[2] + u8"\n可用：字母/数字、f1-f12、space、\nleft.shift、mouse.left 等", 6000);
            return;
        }

        BindableAction* action = BindRegistry::Instance().Find(name);
        if (!action)
        {
            std::string msg = u8"未找到模块：" + name + u8"\n输入 .bind list 查看全部模块";
            NotificationItem::Instance().AddNotification(NotificationType_Warning, msg, 6000);
            return;
        }

        if (action->setKey)
            action->setKey(vk);
        std::string msg = u8"已将 [" + action->label + u8"] 绑定到 " + KeyDisplayName(vk);
        if (vk == 0)
            msg = u8"已解除 [" + action->label + u8"] 的绑定";
        NotificationItem::Instance().AddNotification(NotificationType_Success, msg);
        return;
    }

    if (cmd == ".unbind")
    {
        if (tokens.size() < 2)
        {
            NotificationItem::Instance().AddNotification(NotificationType_Warning,
                u8"用法：.unbind <模块名>", 5000);
            return;
        }
        BindableAction* action = BindRegistry::Instance().Find(tokens[1]);
        if (!action)
        {
            NotificationItem::Instance().AddNotification(NotificationType_Warning,
                u8"未找到模块：" + ToLowerStr(tokens[1]), 5000);
            return;
        }
        if (action->setKey)
            action->setKey(0);
        NotificationItem::Instance().AddNotification(NotificationType_Success,
            u8"已解除 [" + action->label + u8"] 的绑定");
        return;
    }

    if (cmd == ".binds" || cmd == ".bindlist" || cmd == ".help")
    {
        std::string msg = u8"命令：\n.bind <模块> <按键>\n.unbind <模块>\n.bind list";
        NotificationItem::Instance().AddNotification(NotificationType_Info, msg, 6000);
        return;
    }

    // 其他 '.' 开头的内容不是我们的命令，忽略
}
