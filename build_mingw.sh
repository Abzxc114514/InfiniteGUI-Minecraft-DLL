#!/bin/bash
# InfiniteGUI-DLL MinGW-w64 交叉构建脚本（Linux 上构建 Windows x64 DLL）
set -e

SRC=/workspace/InfiniteGUI-DLL
OUT=/workspace/build
OBJ=$OUT/obj
DEPS=/workspace/deps
MINGWHOOK=$DEPS/minhook-1.3.3
GLEW=$DEPS/glew-2.2.0

CXX=x86_64-w64-mingw32-g++
mkdir -p "$OBJ"

# ---- 源码 staging：仓库是 GBK/UTF-8 混合编码，统一转成 UTF-8 后再编译 ----
STAGE=$OUT/src
rm -rf "$STAGE"
cp -r "$SRC" "$STAGE"
find "$STAGE" \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) | while read -r f; do
  if ! iconv -f UTF-8 -t UTF-8 "$f" -o /dev/null 2>/dev/null; then
    if iconv -f GBK -t UTF-8 "$f" -o "$f.tmp" 2>/dev/null; then
      mv "$f.tmp" "$f"
    else
      echo "ENCODE-FAIL: $f"; exit 1
    fi
  fi
done
SRC="$STAGE"

# 源文件列表（与 vcxproj 的 ClCompile 一致）
FILES=(
  App.cpp AudioManager.cpp AutoText.cpp BilibiliFansItem.cpp Blur.cpp
  ClickEffect.cpp ConfigManager.cpp CounterItem.cpp CPSDetector.cpp CPSItem.cpp
  DanmakuItem.cpp dllmain.cpp FileCountItem.cpp fonts.cpp FpsItem.cpp
  GameKeyBind.cpp GameStateDetector.cpp GameWindowTool.cpp GlobalConfig.cpp
  GlobalWindowStyle.cpp gui.cpp HttpClient.cpp HttpUpdateWorker.cpp Images.cpp
  ItemManager.cpp KeyState.cpp KeystrokesItem.cpp Menu.cpp Motionblur.cpp
  MusicInfoItem.cpp Notification.cpp NotificationItem.cpp opengl_hook.cpp
  Sprint.cpp StringConverter.cpp Text.cpp TextItem.cpp TimeItem.cpp
  ChangeLog.cpp WindowSnapper.cpp
  imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp
  imgui/imgui_impl_opengl2.cpp imgui/imgui_impl_opengl3.cpp
  imgui/imgui_impl_win32.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp
)

CXXFLAGS="-std=c++17 -O2 -c \
  -DIGUI_MINGW -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE -DNDEBUG -D_CRT_SECURE_NO_WARNINGS \
  -I$SRC -I$DEPS/casecompat -I$DEPS -I$GLEW/include -I$MINGWHOOK/include \
  -Wno-unknown-pragmas -Wno-attributes -Wno-narrowing"

compile_one() {
  f="$1"
  obj="$OBJ/$(echo "$f" | tr '/' '_').o"
  $CXX $CXXFLAGS "$SRC/$f" -o "$obj"
  echo "OK  $f"
}
export -f compile_one
export OBJ CXX CXXFLAGS SRC

# 并行编译
printf '%s\n' "${FILES[@]}" | xargs -P "$(nproc)" -I{} bash -c 'compile_one "$@"' _ {}

# 链接
echo "=== linking ==="
$CXX -shared -o "$OUT/InfiniteGUI-DLL.dll" "$OBJ"/*.o \
  "$MINGWHOOK/build/libminhook.a" "$OUT/libglew32.dll.a" \
  -lopengl32 -lgdi32 -luser32 -lshell32 -limm32 -ldwmapi -lshlwapi \
  -lwinhttp -lpsapi -lwinmm -lole32 -luuid \
  -static-libgcc -static-libstdc++ \
  -Wl,--subsystem,windows

echo "=== done ==="
ls -la "$OUT/InfiniteGUI-DLL.dll" "$OUT/glew32.dll"
