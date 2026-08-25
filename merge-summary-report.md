本次合并为 InfiniteGUI-DLL 项目新增了两个功能模块——"假防砍"本地装饰动画（FakeBlock）与基于游戏聊天栏的 `.bind` 按键绑定命令系统（ChatCommand + BindRegistry），并引入完整的 MinGW-w64 跨平台交叉编译支持（新增构建脚本、MinHook 替代 Detours 的条件编译、WinRT 代码的条件排除、GBK→UTF-8 编码统一及 include 路径修正）。同时将 GLEW 2.2.0、MinHook 1.3.3、nlohmann/json、stb_image 等第三方依赖源码及 MinGW 构建产物（DLL、源码副本）一并纳入仓库。

| 文件 | 变更 |
|------|---------|
| InfiniteGUI-DLL/ChatCommand.h | - 新增聊天命令模块头文件，定义 `.bind <模块> <按键>` / `.bind list` / `.unbind <模块>` 命令接口（纯本地键盘截获实现，不读写游戏内存） |
| InfiniteGUI-DLL/ChatCommand.cpp | - 新增聊天命令实现：VK 转字符还原输入、按键名解析（支持字母/数字/F1-F12/space/mouse.left 等）及命令执行与通知反馈逻辑 |
| InfiniteGUI-DLL/FakeBlock.h | - 新增"假防砍"模块：按住右键时本地渲染 1.7.10 风格的剑影 + 格挡弧光动画，支持切换键、动画时长、透明度、缩放、颜色等配置项 |
| InfiniteGUI-DLL/FakeBlock.cpp | - 实现假防砍动画的 Update/RenderGui/DrawSword 绘制逻辑及向 BindRegistry 注册可绑定动作 |
| InfiniteGUI-DLL/BindRegistry.h | - 新增可绑定动作注册表单例：支持动作注册/按名查找/按键派发，供 `.bind` 命令查询与修改各模块绑定键 |
| InfiniteGUI-DLL/KeybindModule.h | - 新增 GetBindKey/SetBindKey 方法供聊天命令读写模块绑定键<br>- 修正包含路径大小写（ImguiStd.h → ImGuiStd.h）并修复 GBK 乱码中文注释 |
| InfiniteGUI-DLL/ItemManager.cpp | - 注册 FakeBlock、ChatCommand 两个新模块到模块管理器<br>- 为 sprint（强制疾跑）和 menu（菜单）注册可绑定动作<br>- 修复多处 GBK 乱码注释为 UTF-8 |
| InfiniteGUI-DLL/MusicInfoItem.h | - WinRT 头文件与成员声明用 IGUI_MINGW 宏条件编译，MinGW 构建下自动排除 |
| InfiniteGUI-DLL/MusicInfoItem.cpp | - 系统媒体会话（WinRT SMTC）代码条件编译，MinGW 下仅保留网易云/酷狗窗口标题检测<br>- include 路径反斜杠改正斜杠，std::ifstream 改用 fs::path 打开文件 |
| InfiniteGUI-DLL/detours/titan_hook.h | - 新增 MinGW 分支：Detours 的 MSVC 静态库无法与 MinGW 链接，改用 MinHook 实现同接口的 Hook 封装，MSVC 路径保持不变 |
| InfiniteGUI-DLL/opengl_hook.cpp | - include 路径与文件名大小写修正（menu.h → Menu.h、pics\ → pics/）以兼容 MinGW<br>- InitHook 调用增加 reinterpret_cast 显式转换 |
| InfiniteGUI-DLL/opengl_hook.h | - 新增 `<atomic>` 头文件包含 |
| InfiniteGUI-DLL/InfiniteGUI-DLL.vcxproj | - 工程文件加入 BindRegistry.h、ChatCommand.h、ChatCommand.cpp、FakeBlock.h、FakeBlock.cpp 五个新文件 |
| InfiniteGUI-DLL/DanmakuItem.cpp | - 约 60 处 GBK 乱码中文注释与字符串（弹幕、礼物、开通舰长、进入直播间、点赞等文案）统一转为 UTF-8<br>- include 路径反斜杠改正斜杠 |
| InfiniteGUI-DLL/Anim.h、BilibiliFansItem.cpp、CPSItem.h、ChangeLog.cpp、ClickCircle.h、CounterItem.cpp、FileCountItem.cpp、GameKeyBind.h、ImGuiSty.h、KeystrokesItem.cpp、KeystrokesItem.h、Motionblur.cpp、MyButton.hpp、PanelButton.hpp、Sprint.h、WindowStyleModule.h、fonts.cpp | - 统一 include 路径分隔符与文件名大小写（如 `imgui\imgui.h` → `imgui/imgui.h`），适配 MinGW 交叉编译 |
| build_mingw.sh | - 新增 MinGW-w64 交叉构建脚本：自动将 GBK/UTF-8 混合编码源码转为 UTF-8 staging、按 vcxproj 顺序编译全部源文件并链接 GLEW/MinHook 生成 Windows x64 DLL |
| deps/casecompat/GL/GL.h、deps/casecompat/Windows.h、deps/casecompat/WinUser.h、deps/casecompat/Shlwapi.h | - 新增大小写兼容包装头文件，统一转发到系统小写头文件，解决源码中大小写混用的 include |
| deps/glew-2.2.0/（约 700 个文件） | - 完整引入 GLEW 2.2.0 源码树：src（glew.c/glewinfo.c/visualinfo.c）、include（glew.h/wglew.h/glxew.h/eglew.h）、auto 生成源与扩展定义、config 各平台 Makefile、vc6-vc15 工程及文档 |
| deps/minhook-1.3.3/（约 40 个文件） | - 完整引入 MinHook 1.3.3 源码：hook.c/buffer.c/trampoline.c/hde 反汇编引擎及 VC9-VC15 工程，供 MinGW 构建替代 Detours |
| deps/nlohmann/json.hpp | - 引入 nlohmann/json 单头 JSON 库（24765 行） |
| deps/stb_image.h | - 引入 stb_image 单头图像加载库（7988 行） |
| deps/glew-2.2.0.zip、deps/minhook.zip | - 引入两个依赖库的原始压缩包 |
| build/InfiniteGUI-DLL.dll、build/glew32.dll、build/libglew32.dll.a、build/InfiniteGUI-mingw-build.zip | - 新增 MinGW 交叉编译构建产物：主 DLL、GLEW 运行库与导入库及完整构建打包 |
| build/src/（约 150 个文件） | - 新增构建脚本生成的 UTF-8 编码源码 staging 副本：全部模块源码、imgui 库、detours 头文件、MinHook、miniaudio、字体（阿里巴巴普惠体/图标字体）与图片资源头、vcxproj 工程及 TODO.md |
| .trae-html-share-packages/deps/glew-2.2.0/（19 个 .zip 文件） | - 引入 GLEW 文档 HTML（auto/doc、auto/src、doc 目录）的 zip 打包副本 |
