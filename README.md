# CGGame

> **C++ GDI Game Library** —— 一个纯 GDI 实现的 Windows 2D 游戏引擎。
> API 风格参考 pygame，零外部依赖，仅需 MinGW-w64 + Win32 API，在C++环境下获得类似Python的编写体验。


## 特性

- **纯 Win32** —— 不依赖 SDL / SFML / OpenGL，只用 `gdi32` / `user32`
- **pygame 风格 API** —— `Screen` / `Image` / `Font` / `Event` / `Clock`
- **像素级绘图** —— 直线、矩形、圆、椭圆、多边形、文字、图像
- **图像变换** —— 旋转、翻转、缩放、双线性插值、alpha 混合
- **精灵系统** —— `Sprite` / `Group` / 碰撞检测
- **相机** —— 平移、旋转、缩放、平滑跟随
- **定时器** —— 周期性回调
- **中文支持** —— UTF-8 源码 + 微软雅黑渲染
- **帧率控制** —— `timeBeginPeriod(1)` 精确到毫秒

## 快速开始

### 编译

```bash
g++ main.cpp -o game.exe -std=c++17 -O2 \
    -lgdi32 -luser32 -lwinmm \
    -finput-charset=UTF-8 -fexec-charset=UTF-8
```

### 最小程序

```cpp
#include "cggame.h"

int main() {
    Screen screen(800, 600, rgb(20, 20, 30));
    Clock  clock;

    while (true) {
        for (const auto& e : screen.eventGet()) {
            if (e.type == EVENT_QUIT) return 0;
            if (e.type == EVENT_KEYDOWN && e.key == Key::ESCAPE) return 0;
        }

        screen.fill(rgb(20, 20, 30));
        screen.fillCircle(400, 300, 60, rgb(80, 200, 255));
        screen.update();
        clock.tick(60);
    }
}
```

## 示例

| 示例 | 内容 |
|---|---|
| `01_hello_window` | 打开窗口，画一个圆 |
| `02_sandbox_game` | 一个简单的沙盒游戏 |

![沙盒游戏](docs/images/02_sandbox.png)

## 文档

完整 API 文档见 [docs/api.md](docs/api.md)。

## 依赖

| 依赖 | 说明 |
|---|---|
| C++11 | 结构化绑定、`std::function`、`shared_ptr` |
| Windows API | `gdi32` / `user32` / `winmm` |
| 编译器 | MinGW-w64 GCC 10+ 或 MSVC 2019+ |
| 系统 | Windows 7 及以上 |

## 为什么做这个

想在控制台里做像素游戏，但：
- 用字符当像素，废了我一双手
- SDL / SFML 太重，还要配一堆 DLL
- 直接写 Win32 API 太啰嗦
- pygame 好用但是语言是 Python

于是用 C++ +  GDI 写了一个 pygame 风格的引擎，零依赖，开箱即用。

## 许可证

[MIT](LICENSE)
