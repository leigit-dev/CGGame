# CGGame API 文档

一个纯 GDI 实现的 Windows 控制台 2D 游戏引擎，API 风格参考 pygame。

---

## 目录

- [快速开始](#快速开始)
- [颜色 Pixel](#颜色-pixel)
- [图像 Image](#图像-image)
- [图像变换](#图像变换)
- [绘图原语](#绘图原语)
- [文字 Font](#文字-font)
- [矩形与碰撞 Rect](#矩形与碰撞-rect)
- [屏幕 Screen](#屏幕-screen)
- [事件系统](#事件系统)
- [输入查询](#输入查询)
- [时钟 Clock](#时钟-clock)
- [定时器 Timer](#定时器-timer)
- [相机 Camera2D](#相机-camera2d)
- [精灵 Sprite / Group](#精灵-sprite--group)
- [文件读写](#文件读写)
- [常量速查](#常量速查)
- [完整示例](#完整示例)

---

## 快速开始

**编译命令（MinGW / w64devkit）**：

```bash
g++ main.cpp -o game.exe -std=c++17 -O2 \
    -lgdi32 -luser32 -lwinmm \
    -finput-charset=UTF-8 -fexec-charset=UTF-8
```

**最小程序**：

```cpp
#include "engine.h"

int main() {
    Screen screen(800, 600, rgb(20, 20, 30));
    Clock  clock;

    while (true) {
        // 事件
        for (const auto& e : screen.eventGet()) {
            if (e.type == EVENT_QUIT) return 0;
            if (e.type == EVENT_KEYDOWN && e.key == Key::ESCAPE) return 0;
        }

        // 绘图
        screen.fill(rgb(20, 20, 30));
        screen.fillCircle(400, 300, 60, rgb(80, 200, 255));
        screen.update();

        clock.tick(60);
    }
}
```

---

## 颜色 Pixel

一个 RGBA 像素，各通道 0~255。

### 构造

| 写法 | 说明 |
|---|---|
| `Pixel()` | 默认 `(0,0,0,255)` |
| `Pixel(r, g, b)` | 不透明 |
| `Pixel(r, g, b, a)` | 带透明度 |
| `Pixel(0xFF0000)` | 高位为 0 → `#RRGGBB`，自动补 a=255 |
| `Pixel(0x80FF0000)` | 高位非 0 → `#AARRGGBB` |
| `Pixel::fromRGB(0xFF0000)` | 强制按 `#RRGGBB` |
| `Pixel::fromARGB(0x80FF0000)` | 强制按 `#AARRGGBB` |
| `Pixel::fromHex("#FF0000")` | 字符串 hex（6 位 RGB / 8 位 ARGB） |

### 辅助函数

```cpp
Pixel c1 = rgb(255, 128, 0);           // 不透明橙色
Pixel c2 = rgba(255, 128, 0, 128);     // 半透明
Pixel c3 = hsv(120, 1.0, 1.0);         // 绿色（h: 0~360，s/v/a: 0~1）
Pixel c4 = hsv(300, 0.8, 0.9, 0.5);    // 半透明洋红
Pixel c5 = Pixel::fromHex("#00FF88");
```

### 分量访问

```cpp
Pixel p = rgb(100, 150, 200);
uint8_t r = p.r, g = p.g, b = p.b, a = p.a;
```

---

## 图像 Image

一张可绘制、可贴图的 Surface。

### 构造

```cpp
Image img(64, 64);                     // 64×64 透明
Image img2(200, 100, rgb(40, 50, 80)); // 带背景色

// 从 Pixels 构造（二维数组）
Pixels arr = {
    { rgb(255,0,0), rgb(0,255,0) },
    { rgb(0,0,255), rgb(255,255,0) },
};
Image img3 = loadPixels(arr);
```

### 基本操作

```cpp
img.fill(rgb(30, 30, 30));             // 整屏填充
img.setPixel(10, 20, rgb(255, 0, 0));  // 写像素（带 alpha 混合）
Pixel p = img.getPixel(10, 20);        // 读像素
img.blit(src, x, y);                   // 贴图（带 alpha 混合）
img.fillRect(x, y, w, h, color);       // 矩形填充
Image sub = img.subImage(x, y, w, h);  // 裁剪子图
int w = img.w, h = img.h;              // 尺寸
```

---

## 图像变换

全部是 `Image` 的成员方法。

```cpp
Image r1 = img.rotate(45.0);           // 逆时针 45°，自动扩画布
Image r2 = img.rotate(90.0, false);    // 不扩画布，中心旋转
Image f  = img.flip(true, false);      // 水平翻转（flipX=true）
Image f2 = img.flip(false, true);      // 垂直翻转
Image f3 = img.flip(true, true);       // 两者都翻
Image s  = img.scale(128, 128);        // 缩放（双线性）
Image rz = img.rotozoom(30, 1.5);      // 旋转 + 缩放
```

| 方法 | 参数 | 说明 |
|---|---|---|
| `rotate(deg, expand=true)` | 角度（度），是否扩展画布 | 逆时针为正 |
| `flip(flipX, flipY)` | 两个 bool | 水平 / 垂直翻转 |
| `scale(w, h)` | 目标尺寸 | 双线性插值 |
| `rotozoom(deg, zoom)` | 角度、缩放倍数 | 先缩放再旋转 |

---

## 绘图原语

全部是 `Image` / `Screen` 的成员方法。

```cpp
img.drawLine(x1, y1, x2, y2, color);
img.drawRect(x, y, w, h, color);                 // 矩形描边
img.fillRect(x, y, w, h, color);                 // 矩形填充
img.drawCircle(cx, cy, r, color);                // 圆描边
img.fillCircle(cx, cy, r, color);                // 圆填充
img.drawEllipse(cx, cy, rx, ry, color);          // 椭圆描边
img.fillEllipse(cx, cy, rx, ry, color);          // 椭圆填充
img.drawPolygon(pts, color);                     // 多边形描边
img.fillPolygon(pts, color);                     // 多边形填充
```

**多边形参数**：

```cpp
std::vector<std::pair<int,int>> pts = {
    {100, 50}, {200, 30}, {250, 120}, {150, 180}, {80, 100}
};
img.drawPolygon(pts, rgb(255, 255, 255));
img.fillPolygon(pts, rgba(255, 100, 200, 150));
```

所有绘图函数都带 alpha 混合——`color.a < 255` 时会与背景混色。

---

## 文字 Font

支持中英文，用 Windows GDI 渲染到内存位图。

### 构造

```cpp
Font f1("Consolas", 24);                       // 英文
Font f2("Microsoft YaHei", 18);                // 中文
Font f3("Microsoft YaHei", 32, true);          // 加粗
Font f4(L"微软雅黑", 20, false, true);         // 宽字符 + 斜体
```

### 测量与渲染

```cpp
// 测量文字尺寸，返回 {宽, 高}
std::pair<int,int> sz = font.size("Hello 你好");
int w = sz.first, h = sz.second;

// 渲染成 Image
Image textImg = font.render("Hello", rgb(255, 255, 255));

// 直接画在图上
screen.drawText("你好，世界！", x, y, rgb(255, 255, 255), font);

// 带描边
Image outlined = font.renderOutlined("标题", 
                                     rgb(255, 255, 255),   // 填充色
                                     rgb(0, 0, 0),         // 描边色
                                     2);                    // 描边粗细
```

**推荐做法**：频繁更新的文字（如分数）缓存渲染结果。

```cpp
std::string lastText;
Image textImg;
if (text != lastText) {
    textImg = font.render(text, rgb(255, 255, 255));
    lastText = text;
}
screen.blit(textImg, x, y);
```

---

## 矩形与碰撞 Rect

```cpp
Rect r(100, 100, 50, 30);          // x, y, w, h

// 边和中心
r.left();    r.top();     r.right();   r.bottom();
r.cx();      r.cy();

// 包含 / 相交
r.contains(px, py);                // 点是否在内部
r.contains(other);                 // 矩形是否完整包含另一矩形
r.intersects(other);               // 是否相交
r.intersection(other);             // 返回交集矩形

// 移动与缩放
r.move(dx, dy);                    // 平移
r.moveTo(x, y);                    // 移动到
r.inflate(dw, dh);                 // 从中心扩张/收缩
```

### 碰撞检测

```cpp
rectCollide(a, b);                           // 矩形碰撞
pointInRect(px, py, r);                      // 点在矩形内
circleCollide(cx1, cy1, r1, cx2, cy2, r2);   // 圆碰撞
pointInCircle(px, py, cx, cy, r);            // 点在圆内
```

---

## 屏幕 Screen

继承自 `Image`，额外提供窗口、事件、刷新。

### 构造

```cpp
Screen screen(800, 600);                       // 黑底，自动调整控制台窗口
Screen screen(800, 600, rgb(20, 20, 30));      // 指定背景色
Screen screen(800, 600, rgb(0,0,0), false);    // 不调整窗口
```

构造时自动：
- 调整控制台窗口到 800×600 像素客户区、居中、无滚动条
- 关闭快速编辑、行输入、回显模式
- 隐藏光标
- 创建 DIB 双缓冲

### 刷新

```cpp
screen.update();   // 把后台缓冲刷到窗口
```

### 访问

```cpp
int w = screen.w;     // 屏幕宽
int h = screen.h;     // 屏幕高
```

`Screen` 继承自 `Image`，所以所有绘图方法（`fill` / `blit` / `drawRect` / `fillCircle` / `drawText` 等）都可以直接用。

---

## 事件系统

### 事件类型

| 类型 | 说明 |
|---|---|
| `EVENT_QUIT` | 窗口关闭 |
| `EVENT_KEYDOWN` | 键盘按下 |
| `EVENT_KEYUP` | 键盘抬起 |
| `EVENT_MOUSEDOWN` | 鼠标按下 |
| `EVENT_MOUSEUP` | 鼠标抬起 |
| `EVENT_MOUSEMOVE` | 鼠标移动 |
| `EVENT_MOUSEWHEEL` | 滚轮滚动 |

### Event 字段

| 字段 | 说明 |
|---|---|
| `type` | 事件类型 |
| `key` | 虚拟键码（`KEYDOWN` / `KEYUP`） |
| `mouseX, mouseY` | 鼠标坐标 |
| `button` | `0=左 1=右 2=中` |
| `wheel` | `+1` 上滚 / `-1` 下滚 |

### 使用

```cpp
for (const auto& e : screen.eventGet()) {
    switch (e.type) {
        case EVENT_QUIT:
            running = false;
            break;
        case EVENT_KEYDOWN:
            if (e.key == Key::ESCAPE) running = false;
            if (e.key == Key::SPACE) jump();
            break;
        case EVENT_MOUSEDOWN:
            printf("点击 (%d, %d) 按钮 %d\n", e.mouseX, e.mouseY, e.button);
            break;
        case EVENT_MOUSEWHEEL:
            zoom += e.wheel * 0.1;
            break;
    }
}
```

---

## 输入查询

持续按键用查询式 API，触发式动作用事件。

### 键盘

```cpp
if (screen.keyPressed(Key::A))     player.x -= speed;
if (screen.keyPressed(Key::D))     player.x += speed;
if (screen.keyPressed(Key::SPACE)) jump();

// 修饰键组合
int mods = screen.getMods();
if (mods & KeyMod::SHIFT) speed *= 2;
if (mods & KeyMod::CTRL)  ...
```

### 鼠标

```cpp
bool leftDown = screen.mousePressed(MouseButton::LEFT);
int mx = screen.mouseX();
int my = screen.mouseY();

// 或一次性取坐标
auto pos = screen.mousePos();
int x = pos.first, y = pos.second;
```

---

## 时钟 Clock

精确帧率控制。

```cpp
Clock clock;

while (running) {
    double dt = clock.tick(60);       // 限帧 60 FPS，返回上帧耗时（秒）

    player.x += speed * dt;           // 用 dt 做帧率无关的运动

    printf("FPS = %.1f\n", clock.getFPS());
}
```

| 方法 | 说明 |
|---|---|
| `clock.tick(fps)` | 限帧，返回上帧耗时（秒）。`fps=0` 不限帧 |
| `clock.getFPS()` | 当前帧率 |
| `Clock::getTicks()` | 毫秒时间戳 |
| `Clock::delay(ms)` | 休眠指定毫秒 |

**建议**：所有运动都用 `速度 × dt`，而不是 `每帧固定像素`，这样机器快慢不影响游戏速度。

---

## 定时器 Timer

周期性触发回调。

```cpp
// 每 2 秒触发一次
int id = setTimer(2.0, [](){
    printf("2 秒到了\n");
});

// 5 秒后触发一次
setTimer(5.0, [](){
    printf("一次性\n");
}, false);

// 主循环里必须调用
while (running) {
    double dt = clock.tick(60);
    updateTimers(dt);
    // ...
}

// 取消
cancelTimer(id);
clearTimers();
```

| 函数 | 说明 |
|---|---|
| `setTimer(秒, 回调, repeat=true)` | 创建，返回 id |
| `cancelTimer(id)` | 取消单个 |
| `clearTimers()` | 全部清空 |
| `updateTimers(dt)` | 主循环每帧调用 |

---

## 相机 Camera2D

支持平移、旋转、缩放。

```cpp
Camera2D camera;
camera.screenW = 800;
camera.screenH = 600;
camera.zoom = 1.0;
camera.followSpeed = 8.0;          // 平滑跟随速度（0 = 瞬间）

camera.follow(playerX, playerY);   // 设置跟随目标
camera.snapTo(playerX, playerY);   // 立即对齐，无平滑

// 每帧调用
camera.update(dt);
camera.rotation += 0.01;           // 旋转（弧度，逆时针）
camera.zoom = 1.5;
```

### 坐标转换

```cpp
// 世界 → 屏幕
int sx, sy;
camera.worldToScreen(wx, wy, sx, sy);
auto p = camera.worldToScreen(wx, wy);   // 返回 pair

// 屏幕 → 世界
double wx, wy;
camera.screenToWorld(mx, my, wx, wy);
auto q = camera.screenToWorld(mx, my);
```

### 绘制

```cpp
camera.drawSprite(screen, sprite);                    // 画一个精灵
camera.drawGroup(screen, group);                      // 画整组
camera.drawGrid(screen, 2400, 1800, 200, gridColor);  // 世界网格
camera.drawWorldRect(screen, Rect(0,0,2400,1800), c); // 世界坐标矩形
```

**注意**：HUD 文字、菜单等应该直接画在 `screen` 上（屏幕坐标），不要走相机。

---

## 精灵 Sprite / Group

### Sprite

```cpp
Image img = loadImage("player.bmp");
auto s = std::make_shared<Sprite>(img, 100, 200);

s->vx = 100;      // 速度（像素/秒）
s->vy = 0;
s->ax = 0;        // 加速度（像素/秒²）
s->ay = 500;

// 每帧回调
s->onUpdate = [](Sprite& self, double dt) {
    self.rect.x += 50 * dt;
};

// 自定义绘制
s->onDraw = [](Sprite& self, Image& target) {
    target.blit(self.image, self.rect.x, self.rect.y);
};

s->update(dt);             // 自动应用速度/加速度 + 调用 onUpdate
s->draw(screen);           // 绘制
s->kill();                 // 标记死亡
s->isAlive();
s->centerAt(400, 300);     // 让中心点在指定坐标
```

**Sprite 字段**：

| 字段 | 说明 |
|---|---|
| `image` | 外观 |
| `rect` | 位置与尺寸 |
| `vx, vy` | 速度（像素/秒） |
| `ax, ay` | 加速度 |
| `alive` | 是否存活 |
| `onUpdate` | 每帧回调 `(Sprite&, double dt)` |
| `onDraw` | 绘制回调 `(Sprite&, Image&)` |

### Group

```cpp
Group enemies;
enemies.add(std::make_shared<Sprite>(img, 100, 100));
enemies.add({ s1, s2, s3 });

enemies.update(dt);
enemies.draw(screen);
enemies.killDead();         // 移除已死亡
enemies.size();
enemies.empty();

for (auto& s : enemies) {   // 直接遍历
    if (s->alive) ...
}
```

### 组碰撞

```cpp
// 是否有任何碰撞
if (groupCollide(bullets, enemies)) { ... }

// 所有碰撞对
for (auto& pair : groupCollidePairs(bullets, enemies)) {
    pair.first->kill();     // bullet
    pair.second->kill();    // enemy
    score++;
}

// 组内两两碰撞
for (auto& pair : groupSelfCollide(enemies)) { ... }
```

---

## 文件读写

只支持 24/32 位未压缩 BMP。

```cpp
// 读取
Image img = loadImage("player.bmp");
if (img.w == 0) {
    printf("读取失败\n");
}

// 保存
saveImage(screen, "screenshot.bmp");
```

---

## 常量速查

### Key

```cpp
// 字母与数字
Key::A ... Key::Z
Key::N0 ... Key::N9

// 功能键
Key::ESCAPE   Key::SPACE    Key::RETURN   Key::TAB
Key::BACKSPACE   Key::SHIFT   Key::CTRL    Key::ALT

// 方向键
Key::LEFT   Key::RIGHT   Key::UP   Key::DOWN

// F1~F12
Key::F1 ... Key::F12
```

### KeyMod

```cpp
KeyMod::NONE    // 0
KeyMod::SHIFT   // 1
KeyMod::CTRL    // 2
KeyMod::ALT     // 4
```

### MouseButton

```cpp
MouseButton::LEFT     // 0
MouseButton::RIGHT    // 1
MouseButton::MIDDLE   // 2
```

---

## 完整示例

一个鼠标跟随的粒子游戏。

```cpp
#include "engine.h"
#include <vector>
#include <memory>
using namespace std;

int main() {
    Screen screen(900, 600, rgb(10, 12, 20));
    Clock  clock;
    Font   font("Microsoft YaHei", 16);
    Font   big ("Microsoft YaHei", 28, true);

    // 玩家
    Image playerImg(32, 32, rgba(0,0,0,0));
    playerImg.fillCircle(16, 16, 14, rgb(80, 200, 255));
    playerImg.drawCircle(16, 16, 14, rgb(255, 255, 255));

    auto player = make_shared<Sprite>(playerImg, 400, 300);
    player->onUpdate = [&](Sprite& self, double) {
        self.rect.x += (screen.mouseX() - self.rect.cx()) / 8;
        self.rect.y += (screen.mouseY() - self.rect.cy()) / 8;
    };

    // 粒子
    Group particles;
    Image particleImg(8, 8, rgba(0,0,0,0));
    particleImg.fillCircle(4, 4, 3, rgb(255, 200, 80));

    int spawnTimer = 0;
    int score = 0;
    bool running = true;

    while (running) {
        double dt = clock.tick(60);

        // ---- 事件 ----
        for (const auto& e : screen.eventGet()) {
            if (e.type == EVENT_QUIT) running = false;
            if (e.type == EVENT_KEYDOWN && e.key == Key::ESCAPE) running = false;
        }

        // ---- 每 0.1 秒刷一个粒子 ----
        spawnTimer++;
        if (spawnTimer >= 6) {
            spawnTimer = 0;
            auto p = make_shared<Sprite>(particleImg,
                rand() % 900, rand() % 600);
            p->vx = (rand() % 200 - 100);
            p->vy = -50 - rand() % 100;
            p->ay = 200;
            particles.add(p);
        }

        // ---- 更新 ----
        player->update(dt);
        particles.update(dt);

        // 出屏移除
        for (auto& p : particles)
            if (p->rect.y > 620) p->kill();
        particles.killDead();

        // 玩家碰到粒子 → 得分
        for (auto& p : particles) {
            if (p->alive && rectCollide(player->rect, p->rect)) {
                p->kill();
                score++;
            }
        }

        // ---- 绘制 ----
        screen.fill(rgb(10, 12, 20));

        // 背景网格
        for (int x = 0; x < 900; x += 60)
            screen.drawLine(x, 0, x, 600, rgba(40, 50, 70, 100));
        for (int y = 0; y < 600; y += 60)
            screen.drawLine(0, y, 900, y, rgba(40, 50, 70, 100));

        particles.draw(screen);
        player->draw(screen);

        // HUD
        screen.drawText("分数：" + to_string(score), 10, 10,
                        rgb(255, 255, 255), font);
        screen.drawText("粒子：" + to_string(particles.size()), 10, 34,
                        rgb(200, 220, 255), font);
        screen.drawText("FPS：" + to_string((int)clock.getFPS()),
                        800, 10, rgb(180, 255, 180), font);

        string title = "粒子游戏";
        auto sz = big.size(title);
        screen.drawText(title, (900 - sz.first) / 2, 20,
                        rgb(255, 220, 80), big);

        screen.update();
    }
    return 0;
}
```

---

## 常见问题

**Q: 中文显示成方块怎么办？**
A: 字体换成 `Microsoft YaHei`，编译加 `-finput-charset=UTF-8 -fexec-charset=UTF-8`，源文件保存为 UTF-8。

**Q: 窗口尺寸不对？**
A: `Screen` 构造会用 Terminal 4×6 小字体自动调整，如果系统没有 Terminal 字体则回退到 Consolas。可以用 `Screen(w, h, bg, false)` 关闭自动调整。

**Q: 帧率只有 30？**
A: `Clock` 构造时自动调用了 `timeBeginPeriod(1)`，需要链接 `-lwinmm`。忘记链接会退回到 15.6ms 的默认时钟精度。

**Q: `near` / `small` / `far` 变量名报错？**
A: 这些是 `windows.h` 里的宏。加 `#define NOMINMAX` 可关掉部分，其他只能改变量名。

**Q: 怎么显示控制台调试输出？**
A: 编译时**不加** `-mwindows`，就有控制台窗口，可以直接 `printf`。
