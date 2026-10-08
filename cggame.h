#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <algorithm>
#include <utility>
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <memory>
#include <functional>

// ============================================================
// Pixel —— 单个 RGBA 像素
// ============================================================
struct Pixel {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    Pixel() = default;
    Pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    Pixel(uint32_t v)
        : r((v >> 16) & 0xFF), g((v >> 8) & 0xFF), b(v & 0xFF),
          a((v & 0xFF000000u) ? ((v >> 24) & 0xFF) : 255) {}

    static Pixel fromRGB(uint32_t v) {
        return Pixel((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF, 255);
    }
    static Pixel fromARGB(uint32_t v) {
        return Pixel((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF, (v >> 24) & 0xFF);
    }
    static Pixel fromHex(const char* s) {
        if (!s) return Pixel(0, 0, 0, 0);
        if (s[0] == '#') s++;
        else if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
        auto hex = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        int len = 0;
        while (s[len] && hex(s[len]) >= 0) len++;
        uint32_t v = 0;
        for (int i = 0; i < len; ++i) v = (v << 4) | hex(s[i]);
        if (len == 6) return fromRGB(v);
        if (len == 8) return fromARGB(v);
        std::fprintf(stderr, "[Pixel::fromHex] 非法字符串: %s\n", s);
        return Pixel(0, 0, 0, 0);
    }
    static Pixel fromHex(const std::string& s) { return fromHex(s.c_str()); }
};

inline Pixel rgb (uint8_t r, uint8_t g, uint8_t b)             { return Pixel(r, g, b, 255); }
inline Pixel rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)   { return Pixel(r, g, b, a); }

inline Pixel hsv(double h, double s, double v, double a = 1.0) {
    h /= 360.0;
    h -= std::floor(h);
    s = std::min(1.0, std::max(0.0, s));
    v = std::min(1.0, std::max(0.0, v));
    double c = v * s;
    double x = c * (1 - std::fabs(std::fmod(h * 6, 2) - 1));
    double m = v - c;
    double r = 0, g = 0, b = 0;
    int i = (int)(h * 6);
    switch (i) {
        case 0: r = c; g = x; b = 0; break;
        case 1: r = x; g = c; b = 0; break;
        case 2: r = 0; g = c; b = x; break;
        case 3: r = 0; g = x; b = c; break;
        case 4: r = x; g = 0; b = c; break;
        default: r = c; g = 0; b = x; break;
    }
    return rgba(
        (uint8_t)std::min(255.0, (r + m) * 255 + 0.5),
        (uint8_t)std::min(255.0, (g + m) * 255 + 0.5),
        (uint8_t)std::min(255.0, (b + m) * 255 + 0.5),
        (uint8_t)std::min(255.0, a * 255 + 0.5));
}

using Pixels = std::vector<std::vector<Pixel>>;

inline bool isValidPixels(const Pixels& p) {
    if (p.empty() || p[0].empty()) return false;
    size_t w = p[0].size();
    for (const auto& row : p) if (row.size() != w) return false;
    return true;
}


// ============================================================
// Rect —— 矩形
// ============================================================
struct Rect {
    int x = 0, y = 0, w = 0, h = 0;

    Rect() = default;
    Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}

    int left()   const { return x; }
    int top()    const { return y; }
    int right()  const { return x + w; }
    int bottom() const { return y + h; }
    int cx()     const { return x + w / 2; }
    int cy()     const { return y + h / 2; }

    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    bool contains(const Rect& o) const {
        return o.x >= x && o.y >= y &&
               o.x + o.w <= x + w && o.y + o.h <= y + h;
    }
    bool intersects(const Rect& o) const {
        return !(o.x >= x + w || o.x + o.w <= x ||
                 o.y >= y + h || o.y + o.h <= y);
    }
    Rect intersection(const Rect& o) const {
        int nx = std::max(x, o.x);
        int ny = std::max(y, o.y);
        int nx2 = std::min(x + w, o.x + o.w);
        int ny2 = std::min(y + h, o.y + o.h);
        if (nx2 <= nx || ny2 <= ny) return Rect(0, 0, 0, 0);
        return Rect(nx, ny, nx2 - nx, ny2 - ny);
    }
    void move(int dx, int dy) { x += dx; y += dy; }
    void moveTo(int nx, int ny) { x = nx; y = ny; }
    void inflate(int dw, int dh) {
        x -= dw / 2; y -= dh / 2; w += dw; h += dh;
    }
};

inline bool rectCollide(const Rect& a, const Rect& b) { return a.intersects(b); }
inline bool pointInRect(int px, int py, const Rect& r) { return r.contains(px, py); }
inline bool circleCollide(int cx1, int cy1, int r1,
                          int cx2, int cy2, int r2) {
    int dx = cx1 - cx2, dy = cy1 - cy2, rr = r1 + r2;
    return dx * dx + dy * dy <= rr * rr;
}
inline bool pointInCircle(int px, int py, int cx, int cy, int r) {
    int dx = px - cx, dy = py - cy;
    return dx * dx + dy * dy <= r * r;
}


// ============================================================
// 前置声明
// ============================================================
struct Font;


// ============================================================
// Image —— 可绘制、可贴图的 Surface
// ============================================================
struct Image {
    int w = 0, h = 0;
    Pixels pix;

    Image() = default;
    Image(int width, int height, Pixel bg = Pixel(0, 0, 0, 0))
        : w(width), h(height), pix(height, std::vector<Pixel>(width, bg)) {}

    // ---------- 基本操作 ----------

    void fill(Pixel c) {
        for (auto& row : pix) std::fill(row.begin(), row.end(), c);
    }

    void setPixel(int x, int y, Pixel c) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        if (c.a == 0) return;
        if (c.a == 255) { pix[y][x] = c; return; }
        Pixel& d = pix[y][x];
        uint32_t inv = 255 - c.a;
        d.r = (uint8_t)((c.r * c.a + d.r * inv + 128) >> 8);
        d.g = (uint8_t)((c.g * c.a + d.g * inv + 128) >> 8);
        d.b = (uint8_t)((c.b * c.a + d.b * inv + 128) >> 8);
        d.a = 255;
    }

    Pixel getPixel(int x, int y) const {
        if (x < 0 || y < 0 || x >= w || y >= h) return Pixel(0, 0, 0, 0);
        return pix[y][x];
    }

    void blit(const Image& src, int dx, int dy) {
        for (int sy = 0; sy < src.h; ++sy) {
            int ty = dy + sy;
            if (ty < 0 || ty >= h) continue;
            const auto& srow = src.pix[sy];
            auto&       drow = pix[ty];
            for (int sx = 0; sx < src.w; ++sx) {
                int tx = dx + sx;
                if (tx < 0 || tx >= w) continue;
                const Pixel& s = srow[sx];
                if (s.a == 0) continue;
                if (s.a == 255) { drow[tx] = s; continue; }
                Pixel& d = drow[tx];
                uint32_t inv = 255 - s.a;
                d.r = (uint8_t)((s.r * s.a + d.r * inv + 128) >> 8);
                d.g = (uint8_t)((s.g * s.a + d.g * inv + 128) >> 8);
                d.b = (uint8_t)((s.b * s.a + d.b * inv + 128) >> 8);
                d.a = 255;
            }
        }
    }

    void fillRect(int x, int y, int rw, int rh, Pixel c) {
        if (c.a == 0) return;
        int x0 = std::max(0, x), x1 = std::min(w, x + rw);
        int y0 = std::max(0, y), y1 = std::min(h, y + rh);
        if (c.a == 255) {
            for (int yy = y0; yy < y1; ++yy)
                for (int xx = x0; xx < x1; ++xx)
                    pix[yy][xx] = c;
            return;
        }
        uint32_t inv = 255 - c.a;
        for (int yy = y0; yy < y1; ++yy) {
            for (int xx = x0; xx < x1; ++xx) {
                Pixel& d = pix[yy][xx];
                d.r = (uint8_t)((c.r * c.a + d.r * inv + 128) >> 8);
                d.g = (uint8_t)((c.g * c.a + d.g * inv + 128) >> 8);
                d.b = (uint8_t)((c.b * c.a + d.b * inv + 128) >> 8);
                d.a = 255;
            }
        }
    }

    Image subImage(int x, int y, int sw, int sh) const {
        Image out;
        if (sw <= 0 || sh <= 0) return out;
        out.w = sw;
        out.h = sh;
        out.pix.assign(sh, std::vector<Pixel>(sw, Pixel(0, 0, 0, 0)));
        for (int j = 0; j < sh; ++j) {
            int sy = y + j;
            if (sy < 0 || sy >= h) continue;
            for (int i = 0; i < sw; ++i) {
                int sx = x + i;
                if (sx < 0 || sx >= w) continue;
                out.pix[j][i] = pix[sy][sx];
            }
        }
        return out;
    }

    // ---------- 绘图原语 ----------

    void drawLine(int x1, int y1, int x2, int y2, Pixel c) {
        int dx = std::abs(x2 - x1), dy = -std::abs(y2 - y1);
        int sx = x1 < x2 ? 1 : -1, sy = y1 < y2 ? 1 : -1;
        int err = dx + dy;
        while (true) {
            setPixel(x1, y1, c);
            if (x1 == x2 && y1 == y2) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x1 += sx; }
            if (e2 <= dx) { err += dx; y1 += sy; }
        }
    }

    void drawRect(int x, int y, int rw, int rh, Pixel c) {
        if (rw <= 0 || rh <= 0) return;
        int x2 = x + rw - 1, y2 = y + rh - 1;
        drawLine(x,  y,  x2, y,  c);
        drawLine(x2, y,  x2, y2, c);
        drawLine(x2, y2, x,  y2, c);
        drawLine(x,  y2, x,  y,  c);
    }

    void drawCircle(int cx, int cy, int r, Pixel c) {
        if (r < 0) return;
        int x = r, y = 0, err = 1 - r;
        while (x >= y) {
            setPixel(cx + x, cy + y, c);
            setPixel(cx + y, cy + x, c);
            setPixel(cx - y, cy + x, c);
            setPixel(cx - x, cy + y, c);
            setPixel(cx - x, cy - y, c);
            setPixel(cx - y, cy - x, c);
            setPixel(cx + y, cy - x, c);
            setPixel(cx + x, cy - y, c);
            y++;
            if (err < 0) err += 2 * y + 1;
            else { x--; err += 2 * (y - x) + 1; }
        }
    }

    void fillCircle(int cx, int cy, int r, Pixel c) {
        if (r < 0) return;
        for (int dy = -r; dy <= r; ++dy) {
            int dx = (int)std::sqrt((double)(r * r - dy * dy));
            drawLine(cx - dx, cy + dy, cx + dx, cy + dy, c);
        }
    }

    void drawEllipse(int cx, int cy, int rx, int ry, Pixel c) {
        if (rx <= 0 || ry <= 0) return;
        int steps = std::max(rx, ry) * 4;
        if (steps < 24) steps = 24;
        const double PI2 = 6.283185307179586;
        int px = cx + rx, py = cy;
        for (int i = 1; i <= steps; ++i) {
            double a = PI2 * i / steps;
            int x = cx + (int)(rx * std::cos(a));
            int y = cy + (int)(ry * std::sin(a));
            drawLine(px, py, x, y, c);
            px = x; py = y;
        }
    }

    void fillEllipse(int cx, int cy, int rx, int ry, Pixel c) {
        if (rx <= 0 || ry <= 0) return;
        for (int dy = -ry; dy <= ry; ++dy) {
            double f = 1.0 - (double)(dy * dy) / (double)(ry * ry);
            if (f < 0) continue;
            int dx = (int)(rx * std::sqrt(f));
            drawLine(cx - dx, cy + dy, cx + dx, cy + dy, c);
        }
    }

    void drawPolygon(const std::vector<std::pair<int, int>>& pts, Pixel c) {
        if (pts.size() < 2) return;
        for (size_t i = 0; i < pts.size(); ++i) {
            auto& a = pts[i];
            auto& b = pts[(i + 1) % pts.size()];
            drawLine(a.first, a.second, b.first, b.second, c);
        }
    }

    void fillPolygon(const std::vector<std::pair<int, int>>& pts, Pixel c) {
        if (pts.size() < 3) return;
        int minY = pts[0].second, maxY = pts[0].second;
        for (auto& p : pts) {
            minY = std::min(minY, p.second);
            maxY = std::max(maxY, p.second);
        }
        for (int y = minY; y <= maxY; ++y) {
            std::vector<int> xs;
            for (size_t i = 0; i < pts.size(); ++i) {
                auto& a = pts[i];
                auto& b = pts[(i + 1) % pts.size()];
                int y1 = a.second, y2 = b.second;
                if ((y1 <= y && y2 > y) || (y2 <= y && y1 > y)) {
                    double t = (double)(y - y1) / (y2 - y1);
                    xs.push_back((int)(a.first + t * (b.first - a.first)));
                }
            }
            std::sort(xs.begin(), xs.end());
            for (size_t i = 0; i + 1 < xs.size(); i += 2)
                drawLine(xs[i], y, xs[i + 1], y, c);
        }
    }

    // ---------- 图像变换（声明，实现在后面） ----------
    Image rotate(double degrees, bool expand = true) const;
    Image flip(bool flipX, bool flipY) const;
    Image scale(int newW, int newH) const;
    Image rotozoom(double degrees, double zoom) const;

    // ---------- 文字（声明，实现在 Font 之后） ----------
    void drawText(const std::string& text, int x, int y,
                  Pixel color, const Font& font);
};


// ============================================================
// 从 Pixels 构造 Image
// ============================================================
inline Image loadPixels(const Pixels& arr) {
    Image img;
    if (!isValidPixels(arr)) {
        std::fprintf(stderr, "[loadPixels] ERROR: Pixels 非矩形\n");
        return img;
    }
    img.h = (int)arr.size();
    img.w = (int)arr[0].size();
    img.pix = arr;
    return img;
}


// ============================================================
// 双线性采样（内部用，alpha 预乘）
// ============================================================
inline Pixel sampleBilinear(const Image& img, double x, double y) {
    if (x < -0.5 || y < -0.5 || x > img.w - 0.5 || y > img.h - 0.5)
        return Pixel(0, 0, 0, 0);

    int x0 = (int)std::floor(x);
    int y0 = (int)std::floor(y);
    int x1 = x0 + 1, y1 = y0 + 1;
    double fx = x - x0, fy = y - y0;

    auto get = [&](int px, int py) -> const Pixel& {
        if (px < 0) px = 0;
        if (py < 0) py = 0;
        if (px >= img.w) px = img.w - 1;
        if (py >= img.h) py = img.h - 1;
        return img.pix[py][px];
    };

    const Pixel& p00 = get(x0, y0);
    const Pixel& p10 = get(x1, y0);
    const Pixel& p01 = get(x0, y1);
    const Pixel& p11 = get(x1, y1);

    double w00 = (1 - fx) * (1 - fy), w10 = fx * (1 - fy);
    double w01 = (1 - fx) * fy,       w11 = fx * fy;

    double a00 = p00.a / 255.0, a10 = p10.a / 255.0;
    double a01 = p01.a / 255.0, a11 = p11.a / 255.0;

    double r = p00.r * a00 * w00 + p10.r * a10 * w10
             + p01.r * a01 * w01 + p11.r * a11 * w11;
    double g = p00.g * a00 * w00 + p10.g * a10 * w10
             + p01.g * a01 * w01 + p11.g * a11 * w11;
    double b = p00.b * a00 * w00 + p10.b * a10 * w10
             + p01.b * a01 * w01 + p11.b * a11 * w11;
    double a = a00 * w00 + a10 * w10 + a01 * w01 + a11 * w11;

    if (a < 1e-6) return Pixel(0, 0, 0, 0);

    Pixel out;
    out.r = (uint8_t)std::min(255.0, r / a + 0.5);
    out.g = (uint8_t)std::min(255.0, g / a + 0.5);
    out.b = (uint8_t)std::min(255.0, b / a + 0.5);
    out.a = (uint8_t)std::min(255.0, a * 255 + 0.5);
    return out;
}


// ============================================================
// Image 变换实现
// ============================================================
inline Image Image::rotate(double degrees, bool expand) const {
    if (w == 0 || h == 0) return Image{};
    const double PI = 3.14159265358979323846;
    double rad  = degrees * PI / 180.0;
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);

    int outW, outH;
    if (expand) {
        outW = (int)std::ceil(std::fabs(w * cosA) + std::fabs(h * sinA));
        outH = (int)std::ceil(std::fabs(w * sinA) + std::fabs(h * cosA));
    } else {
        outW = w;
        outH = h;
    }
    if (outW <= 0 || outH <= 0) return Image{};

    Image dst;
    dst.w = outW;
    dst.h = outH;
    dst.pix.assign(outH, std::vector<Pixel>(outW, Pixel(0, 0, 0, 0)));

    double scx = (w - 1) * 0.5, scy = (h - 1) * 0.5;
    double dcx = (outW - 1) * 0.5, dcy = (outH - 1) * 0.5;

    for (int dy = 0; dy < outH; ++dy) {
        for (int dx = 0; dx < outW; ++dx) {
            double rx = dx - dcx, ry = dy - dcy;
            double sx = rx * cosA - ry * sinA + scx;
            double sy = rx * sinA + ry * cosA + scy;
            dst.pix[dy][dx] = sampleBilinear(*this, sx, sy);
        }
    }
    return dst;
}

inline Image Image::flip(bool flipX, bool flipY) const {
    Image dst;
    dst.w = w;
    dst.h = h;
    dst.pix.resize(h);
    for (int y = 0; y < h; ++y) {
        int sy = flipY ? (h - 1 - y) : y;
        dst.pix[y].resize(w);
        const auto& srow = pix[sy];
        auto&       drow = dst.pix[y];
        for (int x = 0; x < w; ++x) {
            int sx = flipX ? (w - 1 - x) : x;
            drow[x] = srow[sx];
        }
    }
    return dst;
}

inline Image Image::scale(int newW, int newH) const {
    Image dst;
    if (w == 0 || h == 0 || newW <= 0 || newH <= 0) return dst;
    dst.w = newW;
    dst.h = newH;
    dst.pix.assign(newH, std::vector<Pixel>(newW, Pixel(0, 0, 0, 0)));
    double rx = (double)w / newW, ry = (double)h / newH;
    for (int dy = 0; dy < newH; ++dy) {
        double sy = (dy + 0.5) * ry - 0.5;
        for (int dx = 0; dx < newW; ++dx) {
            double sx = (dx + 0.5) * rx - 0.5;
            dst.pix[dy][dx] = sampleBilinear(*this, sx, sy);
        }
    }
    return dst;
}

inline Image Image::rotozoom(double degrees, double zoom) const {
    if (zoom <= 0) return Image{};
    Image s = scale(std::max(1, (int)(w * zoom)),
                    std::max(1, (int)(h * zoom)));
    return s.rotate(degrees, true);
}

// ============================================================
// Sprite —— pygame.sprite.Sprite
// ============================================================
struct Sprite {
    Image image;             // 精灵的外观
    Rect  rect;              // 位置和尺寸
    bool  alive = true;
    double vx = 0, vy = 0;   // 速度（像素/秒），自动应用
    double ax = 0, ay = 0;   // 加速度（像素/秒²），自动应用

    // 每帧回调（可选），参数：自己 + 上帧耗时秒
    std::function<void(Sprite&, double)> onUpdate;
    // 绘制回调（可选），不设置就用默认 blit
    std::function<void(Sprite&, Image&)> onDraw;

    Sprite() = default;

    Sprite(const Image& img, int x = 0, int y = 0)
        : image(img), rect(x, y, img.w, img.h) {}

    // 每帧更新位置
    void update(double dt) {
        vx += ax * dt;
        vy += ay * dt;
        rect.x += (int)(vx * dt);
        rect.y += (int)(vy * dt);
        if (onUpdate) onUpdate(*this, dt);
    }

    // 绘制
    void draw(Image& target) {
        if (onDraw) onDraw(*this, target);
        else        target.blit(image, rect.x, rect.y);
    }

    void kill()  { alive = false; }
    bool isAlive() const { return alive; }

    // 便捷：居中对齐
    void centerAt(int x, int y) {
        rect.x = x - rect.w / 2;
        rect.y = y - rect.h / 2;
    }
};


// ============================================================
// Group —— pygame.sprite.Group
// ============================================================
struct Group {
    std::vector<std::shared_ptr<Sprite>> sprites;

    // ---- 增删 ----
    void add(std::shared_ptr<Sprite> s) {
        sprites.push_back(s);
    }
    void add(std::initializer_list<std::shared_ptr<Sprite>> list) {
        for (auto& s : list) sprites.push_back(s);
    }
    void remove(std::shared_ptr<Sprite> s) {
        sprites.erase(std::remove(sprites.begin(), sprites.end(), s),
                      sprites.end());
    }
    void clear() { sprites.clear(); }

    // ---- 批量 update / draw ----
    void update(double dt) {
        for (auto& s : sprites) if (s->alive) s->update(dt);
    }
    void draw(Image& target) {
        for (auto& s : sprites) if (s->alive) s->draw(target);
    }

    // ---- 清掉已死亡的精灵 ----
    void killDead() {
        sprites.erase(
            std::remove_if(sprites.begin(), sprites.end(),
                [](const std::shared_ptr<Sprite>& s){ return !s->alive; }),
            sprites.end());
    }

    // ---- 查询 ----
    size_t size() const { return sprites.size(); }
    bool   empty() const { return sprites.empty(); }

    // 遍历支持：for (auto& s : group) { ... }
    auto begin()       { return sprites.begin(); }
    auto end()         { return sprites.end(); }
    auto begin() const { return sprites.begin(); }
    auto end()   const { return sprites.end(); }
};


// ============================================================
// 组碰撞辅助
// ============================================================

// 两个组之间是否有任何碰撞
inline bool groupCollide(Group& g1, Group& g2) {
    for (auto& a : g1)
        if (a->alive)
            for (auto& b : g2)
                if (b->alive && rectCollide(a->rect, b->rect))
                    return true;
    return false;
}

// 返回所有碰撞的 (a, b) 对
inline std::vector<std::pair<std::shared_ptr<Sprite>,
                             std::shared_ptr<Sprite>>>
groupCollidePairs(Group& g1, Group& g2) {
    std::vector<std::pair<std::shared_ptr<Sprite>,
                          std::shared_ptr<Sprite>>> out;
    for (auto& a : g1)
        if (a->alive)
            for (auto& b : g2)
                if (b->alive && rectCollide(a->rect, b->rect))
                    out.push_back({ a, b });
    return out;
}

// 一组内两两碰撞（不含自己和自己）
inline std::vector<std::pair<std::shared_ptr<Sprite>,
                             std::shared_ptr<Sprite>>>
groupSelfCollide(Group& g) {
    std::vector<std::pair<std::shared_ptr<Sprite>,
                          std::shared_ptr<Sprite>>> out;
    auto& v = g.sprites;
    for (size_t i = 0; i < v.size(); ++i) {
        if (!v[i]->alive) continue;
        for (size_t j = i + 1; j < v.size(); ++j) {
            if (!v[j]->alive) continue;
            if (rectCollide(v[i]->rect, v[j]->rect))
                out.push_back({ v[i], v[j] });
        }
    }
    return out;
}

// ============================================================
// Timer —— 周期性触发
// ============================================================
struct Timer {
    int    id       = 0;
    double interval = 1.0;    // 触发间隔（秒）
    double elapsed  = 0.0;
    bool   repeat   = true;
    bool   alive    = true;
    std::function<void()> callback;
};

struct TimerManager {
    std::vector<Timer> timers;
    int nextId = 1;

    // 添加定时器，返回 id
    int add(double interval, std::function<void()> cb, bool repeat = true) {
        Timer t;
        t.id       = nextId++;
        t.interval = interval;
        t.callback = cb;
        t.repeat   = repeat;
        timers.push_back(t);
        return t.id;
    }

    // 取消定时器
    void cancel(int id) {
        for (auto& t : timers)
            if (t.id == id) t.alive = false;
    }

    // 全部清空
    void clear() { timers.clear(); }

    // 每帧调用
    void update(double dt) {
        // 快照，防止回调中修改容器导致的迭代器失效
        std::vector<Timer> snapshot = timers;

        for (auto& t : snapshot) {
            if (!t.alive) continue;
            t.elapsed += dt;
            while (t.elapsed >= t.interval && t.alive) {
                t.elapsed -= t.interval;
                if (t.callback) t.callback();
                if (!t.repeat) { t.alive = false; break; }
            }
            // 写回真正的定时器
            for (auto& r : timers) {
                if (r.id == t.id) {
                    r.elapsed = t.elapsed;
                    if (!t.alive) r.alive = false;
                    break;
                }
            }
        }

        timers.erase(
            std::remove_if(timers.begin(), timers.end(),
                [](const Timer& t){ return !t.alive; }),
            timers.end());
    }
};

inline TimerManager g_timers;

// 全局便捷函数
inline int  setTimer(double interval,
                     std::function<void()> cb,
                     bool repeat = true) {
    return g_timers.add(interval, cb, repeat);
}
inline void cancelTimer(int id)   { g_timers.cancel(id); }
inline void clearTimers()         { g_timers.clear(); }
inline void updateTimers(double dt) { g_timers.update(dt); }


// ============================================================
// Camera2D —— 世界坐标 ↔ 屏幕坐标，支持平移 / 旋转 / 缩放
// ============================================================
struct Camera2D {
    double x = 0, y = 0;              // 相机中心在世界中的坐标
    double rotation = 0;              // 弧度，逆时针为正
    double zoom = 1.0;                // 缩放
    int    screenW = 800, screenH = 600;

    // 平滑跟随
    double targetX = 0, targetY = 0;
    double followSpeed = 8.0;         // 越大越紧跟
    bool   smoothFollow = true;

    // ---- 设置跟随目标 ----
    void follow(double tx, double ty) {
        targetX = tx;
        targetY = ty;
        if (!smoothFollow) { x = tx; y = ty; }
    }

    // 立即对齐目标，无平滑
    void snapTo(double tx, double ty) { x = tx; y = ty; targetX = tx; targetY = ty; }

    // ---- 每帧调用（用于平滑跟随）----
    void update(double dt) {
        if (smoothFollow) {
            double t = std::min(1.0, followSpeed * dt);
            x += (targetX - x) * t;
            y += (targetY - y) * t;
        }
    }

    // ---- 世界坐标 → 屏幕坐标 ----
    void worldToScreen(double wx, double wy, int& sx, int& sy) const {
        double dx = wx - x;
        double dy = wy - y;
        double c = std::cos(-rotation);
        double s = std::sin(-rotation);
        double rx = (dx * c - dy * s) * zoom;
        double ry = (dx * s + dy * c) * zoom;
        sx = (int)(screenW * 0.5 + rx);
        sy = (int)(screenH * 0.5 + ry);
    }

    std::pair<int,int> worldToScreen(double wx, double wy) const {
        int sx, sy; worldToScreen(wx, wy, sx, sy);
        return { sx, sy };
    }

    // ---- 屏幕坐标 → 世界坐标（鼠标点击常用）----
    void screenToWorld(int sx, int sy, double& wx, double& wy) const {
        double rx = (sx - screenW * 0.5) / zoom;
        double ry = (sy - screenH * 0.5) / zoom;
        double c = std::cos(rotation);
        double s = std::sin(rotation);
        wx = x + rx * c - ry * s;
        wy = y + rx * s + ry * c;
    }

    std::pair<double,double> screenToWorld(int sx, int sy) const {
        double wx, wy; screenToWorld(sx, sy, wx, wy);
        return { wx, wy };
    }

    // ---- 绘制一个 Sprite（自动应用相机变换）----
    void drawSprite(Image& target, const Sprite& s) const {
        double cx = s.rect.x + s.rect.w * 0.5;
        double cy = s.rect.y + s.rect.h * 0.5;

        int sx, sy;
        worldToScreen(cx, cy, sx, sy);

        // 屏幕外粗剔除
        int margin = (int)(std::sqrt(s.rect.w * s.rect.w +
                                     s.rect.h * s.rect.h) * zoom) + 4;
        if (sx < -margin || sx > screenW + margin ||
            sy < -margin || sy > screenH + margin) return;

        // 复制 → 缩放 → 旋转
        Image transformed = s.image;
        if (zoom != 1.0) {
            int nw = std::max(1, (int)(transformed.w * zoom));
            int nh = std::max(1, (int)(transformed.h * zoom));
            transformed = transformed.scale(nw, nh);
        }
        if (rotation != 0) {
            double deg = rotation * 180.0 / 3.14159265358979323846;
            transformed = transformed.rotate(deg, false);
        }

        target.blit(transformed,
                    sx - transformed.w / 2,
                    sy - transformed.h / 2);
    }

    // ---- 绘制一组 Sprite ----
    void drawGroup(Image& target, Group& g) const {
        for (auto& s : g)
            if (s->alive) drawSprite(target, *s);
    }

    // ---- 画世界网格 ----
    void drawGrid(Image& target, int worldW, int worldH,
                  int cell = 100, Pixel c = rgba(60, 80, 120, 100)) const {
        for (int gx = 0; gx <= worldW; gx += cell) {
            int x1, y1, x2, y2;
            worldToScreen(gx, 0,      x1, y1);
            worldToScreen(gx, worldH, x2, y2);
            target.drawLine(x1, y1, x2, y2, c);
        }
        for (int gy = 0; gy <= worldH; gy += cell) {
            int x1, y1, x2, y2;
            worldToScreen(0,      gy, x1, y1);
            worldToScreen(worldW, gy, x2, y2);
            target.drawLine(x1, y1, x2, y2, c);
        }
    }

    // ---- 画世界矩形（世界坐标 → 屏幕坐标）----
    void drawWorldRect(Image& target, const Rect& r, Pixel c) const {
        int x1, y1, x2, y2, x3, y3, x4, y4;
        worldToScreen(r.x,         r.y,         x1, y1);
        worldToScreen(r.x + r.w,   r.y,         x2, y2);
        worldToScreen(r.x + r.w,   r.y + r.h,   x3, y3);
        worldToScreen(r.x,         r.y + r.h,   x4, y4);
        target.drawLine(x1, y1, x2, y2, c);
        target.drawLine(x2, y2, x3, y3, c);
        target.drawLine(x3, y3, x4, y4, c);
        target.drawLine(x4, y4, x1, y1, c);
    }
};
// ============================================================
// 编码转换工具（放在 Font 之前）
// ============================================================
inline std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(),
                                  nullptr, 0);
    if (len <= 0) return L"";
    std::wstring out(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(),
                        &out[0], len);
    return out;
}

inline std::string wideToUtf8(const std::wstring& s) {
    if (s.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(),
                                  nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(),
                        &out[0], len, nullptr, nullptr);
    return out;
}


// ============================================================
// Font —— 文字渲染（支持中英文）
// ============================================================
struct Font {
    std::wstring face   = L"Microsoft YaHei";   // 默认微软雅黑
    int          ptSize = 24;
    bool         bold   = false;
    bool         italic = false;

    Font() = default;

    // 从窄字符串构造（字体名按 UTF-8 解释）
    Font(const std::string& face_, int size_,
         bool bold_ = false, bool italic_ = false)
        : ptSize(size_), bold(bold_), italic(italic_) {
        face = utf8ToWide(face_);
    }

    // 从宽字符串构造（直接给宽字符字体名，比如 L"微软雅黑"）
    Font(const std::wstring& face_, int size_,
         bool bold_ = false, bool italic_ = false)
        : face(face_), ptSize(size_), bold(bold_), italic(italic_) {}

    // ---- 创建 GDI 字体 ----
    HFONT makeFont() const {
        return CreateFontW(
            ptSize, 0, 0, 0,
            bold ? FW_BOLD : FW_NORMAL,
            italic ? TRUE : FALSE,
            FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            face.c_str());
    }

    // ---- 测量文字尺寸（返回像素宽高，不含边距）----
    std::pair<int, int> size(const std::string& text) const {
        return sizeW(utf8ToWide(text));
    }
    std::pair<int, int> sizeW(const std::wstring& text) const {
        if (text.empty()) return { 0, 0 };

        HDC screenDC = GetDC(nullptr);
        HFONT hfont = makeFont();
        HDC tmpDC = CreateCompatibleDC(screenDC);
        HFONT oldFont = (HFONT)SelectObject(tmpDC, hfont);

        SIZE sz = { 0, 0 };
        GetTextExtentPoint32W(tmpDC, text.c_str(),
                              (int)text.size(), &sz);

        SelectObject(tmpDC, oldFont);
        DeleteObject(hfont);
        DeleteDC(tmpDC);
        ReleaseDC(nullptr, screenDC);
        return { sz.cx, sz.cy };
    }

    // ---- 渲染为 Image ----
    Image render(const std::string& text,
                 Pixel color = rgb(255, 255, 255)) const {
        return renderW(utf8ToWide(text), color);
    }

    Image renderW(const std::wstring& text,
                  Pixel color = rgb(255, 255, 255)) const {
        Image img;
        if (text.empty()) return img;

        HDC screenDC = GetDC(nullptr);
        HFONT hfont = makeFont();
        HDC tmpDC = CreateCompatibleDC(screenDC);
        HFONT oldFont = (HFONT)SelectObject(tmpDC, hfont);

        SIZE sz = { 0, 0 };
        GetTextExtentPoint32W(tmpDC, text.c_str(),
                              (int)text.size(), &sz);

        int tw = sz.cx + 4;   // 左右各留 2px 抗锯齿余量
        int th = sz.cy + 4;
        if (tw < 1) tw = 1;
        if (th < 1) th = 1;

        // 创建 32 位 DIB 作为渲染目标
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = tw;
        bmi.bmiHeader.biHeight      = -th;   // top-down
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        HBITMAP dib = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS,
                                       &bits, nullptr, 0);
        HBITMAP oldBmp = (HBITMAP)SelectObject(tmpDC, dib);

        // 清空为全黑（GDI 在黑色背景上绘制文字）
        std::memset(bits, 0, (size_t)tw * th * 4);

        // 设置文字绘制参数
        SetBkMode(tmpDC, TRANSPARENT);
        SetTextColor(tmpDC, RGB(color.r, color.g, color.b));
        TextOutW(tmpDC, 2, 2, text.c_str(), (int)text.size());

        // 从 DIB 读取像素，反推 alpha 覆盖率
        img.w = tw;
        img.h = th;
        img.pix.assign(th, std::vector<Pixel>(tw));

        uint32_t* src = (uint32_t*)bits;
        int maxChannel = std::max({ (int)color.r,
                                     (int)color.g,
                                     (int)color.b });

        for (int y = 0; y < th; ++y) {
            for (int x = 0; x < tw; ++x) {
                uint32_t v = src[y * tw + x];
                int r = (v >> 16) & 0xFF;
                int g = (v >>  8) & 0xFF;
                int b =  v        & 0xFF;

                if (maxChannel == 0) {
                    img.pix[y][x] = Pixel(0, 0, 0, 0);
                    continue;
                }
                int got = std::max({ r, g, b });
                if (got == 0) {
                    img.pix[y][x] = Pixel(0, 0, 0, 0);
                    continue;
                }
                // 覆盖率 = 实际亮度 / 最大通道亮度
                float cov = (float)got / maxChannel;
                if (cov > 1.0f) cov = 1.0f;
                img.pix[y][x] = rgba(color.r, color.g, color.b,
                                     (uint8_t)(cov * 255));
            }
        }

        // 清理
        SelectObject(tmpDC, oldBmp);
        SelectObject(tmpDC, oldFont);
        DeleteObject(dib);
        DeleteObject(hfont);
        DeleteDC(tmpDC);
        ReleaseDC(nullptr, screenDC);
        return img;
    }

    // ---- 带描边渲染（用 8 个方向偏移画外框，再画内部）----
    Image renderOutlined(const std::string& text,
                         Pixel fillColor,
                         Pixel outlineColor,
                         int outlineThickness = 2) const {
        return renderOutlinedW(utf8ToWide(text), fillColor,
                               outlineColor, outlineThickness);
    }

    Image renderOutlinedW(const std::wstring& text,
                          Pixel fillColor,
                          Pixel outlineColor,
                          int outlineThickness = 2) const {
        if (text.empty()) return Image{};
        if (outlineThickness <= 0)
            return renderW(text, fillColor);

        Image outline = renderW(text, outlineColor);
        Image fill    = renderW(text, fillColor);

        int pad = outlineThickness;
        Image result(outline.w + pad * 2,
                     outline.h + pad * 2,
                     rgba(0, 0, 0, 0));

        // 描边：8 个方向偏移叠加
        int dirs[8][2] = {
            {-1,-1}, {0,-1}, {1,-1},
            {-1, 0},          {1, 0},
            {-1, 1}, {0, 1}, {1, 1}
        };
        for (auto& d : dirs) {
            int ox = pad + d[0] * outlineThickness;
            int oy = pad + d[1] * outlineThickness;
            result.blit(outline, ox, oy);
        }

        // 内部填充
        result.blit(fill, pad, pad);
        return result;
    }
};


// ============================================================
// Image::drawText 实现（Font 已定义）
// ============================================================
inline void Image::drawText(const std::string& text, int x, int y,
                            Pixel color, const Font& font) {
    Image t = font.render(text, color);
    blit(t, x, y);
}


// ============================================================
// BMP 读写
// ============================================================
inline Image loadImage(const std::string& path) {
    Image img;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        std::fprintf(stderr, "[loadImage] 打开失败: %s\n", path.c_str());
        return img;
    }

    BITMAPFILEHEADER fh;
    if (fread(&fh, sizeof(fh), 1, f) != 1 || fh.bfType != 0x4D42) {
        std::fprintf(stderr, "[loadImage] 不是 BMP: %s\n", path.c_str());
        fclose(f);
        return img;
    }

    BITMAPINFOHEADER ih;
    if (fread(&ih, sizeof(ih), 1, f) != 1) { fclose(f); return img; }
    if ((ih.biBitCount != 24 && ih.biBitCount != 32) || ih.biCompression != 0) {
        std::fprintf(stderr, "[loadImage] 只支持 24/32 位未压缩 BMP\n");
        fclose(f);
        return img;
    }

    int bpp = ih.biBitCount / 8;
    img.w = (int)ih.biWidth;
    img.h = (int)(ih.biHeight < 0 ? -ih.biHeight : ih.biHeight);
    bool topDown = ih.biHeight < 0;
    int rowSize = ((img.w * ih.biBitCount + 31) / 32) * 4;
    std::vector<uint8_t> row(rowSize);

    fseek(f, fh.bfOffBits, SEEK_SET);
    img.pix.assign(img.h, std::vector<Pixel>(img.w));
    for (int y = 0; y < img.h; ++y) {
        if (fread(row.data(), rowSize, 1, f) != 1) break;
        int dstY = topDown ? y : (img.h - 1 - y);
        for (int x = 0; x < img.w; ++x) {
            uint8_t b = row[x * bpp + 0];
            uint8_t g = row[x * bpp + 1];
            uint8_t r = row[x * bpp + 2];
            uint8_t a = (bpp == 4) ? row[x * bpp + 3] : 255;
            img.pix[dstY][x] = rgba(r, g, b, a);
        }
    }
    fclose(f);
    return img;
}

inline bool saveImage(const Image& img, const std::string& path) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    int rowSize = ((img.w * 24 + 31) / 32) * 4;
    uint32_t dataSize = (uint32_t)rowSize * img.h;
    uint32_t fileSize = 14 + 40 + dataSize;

    BITMAPFILEHEADER fh = {};
    fh.bfType    = 0x4D42;
    fh.bfSize    = fileSize;
    fh.bfOffBits = 14 + 40;
    fwrite(&fh, sizeof(fh), 1, f);

    BITMAPINFOHEADER ih = {};
    ih.biSize        = sizeof(BITMAPINFOHEADER);
    ih.biWidth       = img.w;
    ih.biHeight      = img.h;
    ih.biPlanes      = 1;
    ih.biBitCount    = 24;
    ih.biSizeImage   = dataSize;
    fwrite(&ih, sizeof(ih), 1, f);

    std::vector<uint8_t> row(rowSize, 0);
    for (int y = img.h - 1; y >= 0; --y) {
        for (int x = 0; x < img.w; ++x) {
            const Pixel& p = img.pix[y][x];
            row[x * 3 + 0] = p.b;
            row[x * 3 + 1] = p.g;
            row[x * 3 + 2] = p.r;
        }
        fwrite(row.data(), rowSize, 1, f);
    }
    fclose(f);
    return true;
}


// ============================================================
// 键鼠常量
// ============================================================
namespace Key {
    constexpr int A = 'A', B = 'B', C = 'C', D = 'D', E = 'E', F = 'F', G = 'G',
                  H = 'H', I = 'I', J = 'J', K = 'K', L = 'L', M = 'M', N = 'N',
                  O = 'O', P = 'P', Q = 'Q', R = 'R', S = 'S', T = 'T', U = 'U',
                  V = 'V', W = 'W', X = 'X', Y = 'Y', Z = 'Z';
    constexpr int N0 = '0', N1 = '1', N2 = '2', N3 = '3', N4 = '4',
                  N5 = '5', N6 = '6', N7 = '7', N8 = '8', N9 = '9';
    constexpr int ESCAPE = VK_ESCAPE, SPACE = VK_SPACE, RETURN = VK_RETURN,
                  TAB = VK_TAB, BACKSPACE = VK_BACK, SHIFT = VK_SHIFT,
                  CTRL = VK_CONTROL, ALT = VK_MENU;
    constexpr int LEFT = VK_LEFT, RIGHT = VK_RIGHT, UP = VK_UP, DOWN = VK_DOWN;
    constexpr int F1 = VK_F1, F2 = VK_F2, F3 = VK_F3, F4 = VK_F4, F5 = VK_F5,
                  F6 = VK_F6, F7 = VK_F7, F8 = VK_F8, F9 = VK_F9,
                  F10 = VK_F10, F11 = VK_F11, F12 = VK_F12;
}

namespace KeyMod {
    constexpr int NONE  = 0;
    constexpr int SHIFT = 1;
    constexpr int CTRL  = 2;
    constexpr int ALT   = 4;
}

namespace MouseButton {
    constexpr int LEFT = 0, RIGHT = 1, MIDDLE = 2;
}


// ============================================================
// 事件
// ============================================================
enum EventType {
    EVENT_NONE = 0, EVENT_QUIT,
    EVENT_KEYDOWN, EVENT_KEYUP,
    EVENT_MOUSEDOWN, EVENT_MOUSEUP, EVENT_MOUSEMOVE, EVENT_MOUSEWHEEL,
};

struct Event {
    EventType type   = EVENT_NONE;
    int       key    = 0;
    int       mouseX = 0;
    int       mouseY = 0;
    int       button = 0;
    int       wheel  = 0;
};

namespace detail {
    inline bool g_prevKey[256] = {}, g_currKey[256] = {};
    inline bool g_prevMouse[3] = {}, g_currMouse[3] = {};
    inline int  g_prevMouseX = 0, g_currMouseX = 0;
    inline int  g_prevMouseY = 0, g_currMouseY = 0;
    inline bool g_initialized = false;

    inline void pollKeyMouse() {
        memcpy(g_prevKey, g_currKey, sizeof(g_currKey));
        for (int vk = 0; vk < 256; ++vk)
            g_currKey[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;

        memcpy(g_prevMouse, g_currMouse, sizeof(g_currMouse));
        g_currMouse[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        g_currMouse[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        g_currMouse[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;

        g_prevMouseX = g_currMouseX;
        g_prevMouseY = g_currMouseY;
        POINT p;
        GetCursorPos(&p);
        HWND hwnd = GetConsoleWindow();
        if (hwnd) ScreenToClient(hwnd, &p);
        g_currMouseX = p.x;
        g_currMouseY = p.y;
    }
}

inline void noedit() {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    mode &= ~ENABLE_INSERT_MODE;
    mode &= ~ENABLE_MOUSE_INPUT;
    mode &= ~ENABLE_LINE_INPUT;
    mode &= ~ENABLE_ECHO_INPUT;
    mode &= ~ENABLE_PROCESSED_INPUT;
    SetConsoleMode(hStdin, mode);
}


// ============================================================
// Clock
// ============================================================
struct Clock {
    LARGE_INTEGER freq, last, fpsMark;
    int    frameCount = 0;
    double fpsValue = 0;

    Clock() {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&last);
        fpsMark = last;
        timeBeginPeriod(1);
    }
    ~Clock() {
        timeEndPeriod(1);
    }

    double tick(double targetFPS = 60) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double dt = (double)(now.QuadPart - last.QuadPart) / freq.QuadPart;
        last = now;

        if (targetFPS > 0) {
            double target = 1.0 / targetFPS;
            double remain = target - dt;
            if (remain > 0.0005) {
                Sleep((DWORD)(remain * 1000));
                QueryPerformanceCounter(&last);
            }
        }

        ++frameCount;
        double elapsed = (double)(now.QuadPart - fpsMark.QuadPart) / freq.QuadPart;
        if (elapsed >= 1.0) {
            fpsValue = frameCount / elapsed;
            frameCount = 0;
            fpsMark = now;
        }
        return dt;
    }

    double getFPS() const { return fpsValue; }

    static uint64_t getTicks() { return GetTickCount64(); }
    static void     delay(int ms) { Sleep(ms); }
};


// ============================================================
// Screen —— 继承 Image，额外提供窗口和事件
// ============================================================
struct Screen : Image {
    HWND    hwnd   = nullptr;
    HDC     hdc    = nullptr;
    HDC     memDC  = nullptr;
    HBITMAP hDib   = nullptr, oldBmp = nullptr;
    void*   dibPix = nullptr;
    std::vector<Event> events;

    static void fitWindow(int clientW, int clientH, bool center = true) {
        HWND hwnd = GetConsoleWindow();
        if (!hwnd) return;
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

        CONSOLE_FONT_INFOEX cfi = {};
        cfi.cbSize       = sizeof(cfi);
        cfi.dwFontSize.X = 4;
        cfi.dwFontSize.Y = 6;
        cfi.FontFamily   = FF_DONTCARE;
        cfi.FontWeight   = FW_NORMAL;
        wcscpy(cfi.FaceName, L"Terminal");
        if (!SetCurrentConsoleFontEx(hOut, FALSE, &cfi)) {
            wcscpy(cfi.FaceName, L"Consolas");
            cfi.dwFontSize.X = 0;
            cfi.dwFontSize.Y = 8;
            SetCurrentConsoleFontEx(hOut, FALSE, &cfi);
        }

        CONSOLE_FONT_INFO cfInfo;
        GetCurrentConsoleFont(hOut, FALSE, &cfInfo);
        COORD fp = GetConsoleFontSize(hOut, cfInfo.nFont);
        int fw = fp.X > 0 ? fp.X : 8;
        int fh = fp.Y > 0 ? fp.Y : 12;

        int cols = (clientW + fw - 1) / fw;
        int rows = (clientH + fh - 1) / fh;
        if (cols < 1) cols = 1;
        if (rows < 1) rows = 1;

        SetConsoleScreenBufferSize(hOut, {(SHORT)cols, (SHORT)rows});
        SMALL_RECT win = {0, 0, (SHORT)(cols - 1), (SHORT)(rows - 1)};
        SetConsoleWindowInfo(hOut, TRUE, &win);
        SetConsoleScreenBufferSize(hOut, {(SHORT)cols, (SHORT)rows});

        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        style &= ~(WS_VSCROLL | WS_HSCROLL);
        SetWindowLong(hwnd, GWL_STYLE, style);

        int realW = cols * fw, realH = rows * fh;
        RECT rc = {0, 0, realW, realH};
        AdjustWindowRect(&rc, style, FALSE);
        int winW = rc.right - rc.left, winH = rc.bottom - rc.top;

        int x = 0, y = 0;
        if (center) {
            int sw = GetSystemMetrics(SM_CXSCREEN);
            int sh = GetSystemMetrics(SM_CYSCREEN);
            if (winW < sw && winH < sh) {
                x = (sw - winW) / 2;
                y = (sh - winH) / 2;
            }
        }
        SetWindowPos(hwnd, NULL, x, y, winW, winH,
                     SWP_NOZORDER | SWP_SHOWWINDOW);
        SetWindowLong(hwnd, GWL_STYLE, style);
        ShowScrollBar(hwnd, SB_BOTH, FALSE);
    }

    Screen(int width, int height, Pixel bg = rgb(0, 0, 0), bool doFit = true) {
        w = width;
        h = height;
        pix.assign(height, std::vector<Pixel>(width, bg));

        hwnd = GetConsoleWindow();
        if (!hwnd) return;
        if (doFit) fitWindow(width, height);
        noedit();

        CONSOLE_CURSOR_INFO ci = {1, 0};
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);

        hdc = GetDC(hwnd);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = w;
        bmi.bmiHeader.biHeight      = -h;
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        hDib = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &dibPix, nullptr, 0);
        memDC = CreateCompatibleDC(hdc);
        oldBmp = (HBITMAP)SelectObject(memDC, hDib);
        detail::g_initialized = false;
    }

    ~Screen() {
        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
        DWORD mode;
        GetConsoleMode(hStdin, &mode);
        mode |= ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE
              | ENABLE_MOUSE_INPUT | ENABLE_LINE_INPUT
              | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT;
        SetConsoleMode(hStdin, mode);

        CONSOLE_CURSOR_INFO ci = {1, 1};
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);

        if (memDC) { SelectObject(memDC, oldBmp); DeleteDC(memDC); }
        if (hDib)  DeleteObject(hDib);
        if (hdc)   ReleaseDC(hwnd, hdc);
    }

    const std::vector<Event>& eventGet() {
        events.clear();

        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                events.push_back({EVENT_QUIT});
            }
            else if (msg.message == WM_MOUSEWHEEL) {
                int delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
                POINT p;
                p.x = GET_X_LPARAM(msg.lParam);
                p.y = GET_Y_LPARAM(msg.lParam);
                ScreenToClient(hwnd, &p);
                Event e;
                e.type   = EVENT_MOUSEWHEEL;
                e.mouseX = p.x;
                e.mouseY = p.y;
                e.wheel  = delta / WHEEL_DELTA;
                events.push_back(e);
            }
            else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (!detail::g_initialized) {
            for (int vk = 0; vk < 256; ++vk)
                detail::g_currKey[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
            detail::g_currMouse[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            detail::g_currMouse[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
            detail::g_currMouse[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
            POINT p;
            GetCursorPos(&p);
            if (hwnd) ScreenToClient(hwnd, &p);
            detail::g_currMouseX = p.x;
            detail::g_currMouseY = p.y;
            detail::g_initialized = true;
        }

        detail::pollKeyMouse();

        if (!IsWindow(hwnd)) {
            events.push_back({EVENT_QUIT});
            return events;
        }

        for (int vk = 0; vk < 256; ++vk) {
            bool prev = detail::g_prevKey[vk];
            bool curr = detail::g_currKey[vk];
            if (!prev && curr) {
                Event e; e.type = EVENT_KEYDOWN; e.key = vk;
                events.push_back(e);
            }
            else if (prev && !curr) {
                Event e; e.type = EVENT_KEYUP; e.key = vk;
                events.push_back(e);
            }
        }

        for (int b = 0; b < 3; ++b) {
            bool prev = detail::g_prevMouse[b];
            bool curr = detail::g_currMouse[b];
            if (!prev && curr) {
                Event e;
                e.type = EVENT_MOUSEDOWN;
                e.mouseX = detail::g_currMouseX;
                e.mouseY = detail::g_currMouseY;
                e.button = b;
                events.push_back(e);
            }
            else if (prev && !curr) {
                Event e;
                e.type = EVENT_MOUSEUP;
                e.mouseX = detail::g_currMouseX;
                e.mouseY = detail::g_currMouseY;
                e.button = b;
                events.push_back(e);
            }
        }

        if (detail::g_currMouseX != detail::g_prevMouseX ||
            detail::g_currMouseY != detail::g_prevMouseY) {
            Event e;
            e.type   = EVENT_MOUSEMOVE;
            e.mouseX = detail::g_currMouseX;
            e.mouseY = detail::g_currMouseY;
            events.push_back(e);
        }

        return events;
    }

    bool keyPressed(int vk) const {
        return (vk >= 0 && vk <= 255) ? detail::g_currKey[vk] : false;
    }
    bool mousePressed(int btn) const {
        return (btn >= 0 && btn <= 2) ? detail::g_currMouse[btn] : false;
    }
    int mouseX() const { return detail::g_currMouseX; }
    int mouseY() const { return detail::g_currMouseY; }
    std::pair<int, int> mousePos() const {
        return { detail::g_currMouseX, detail::g_currMouseY };
    }

    int getMods() const {
        int m = KeyMod::NONE;
        if (GetAsyncKeyState(VK_SHIFT)   & 0x8000) m |= KeyMod::SHIFT;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) m |= KeyMod::CTRL;
        if (GetAsyncKeyState(VK_MENU)    & 0x8000) m |= KeyMod::ALT;
        return m;
    }

    void update() {
        uint32_t* dst = (uint32_t*)dibPix;
        for (int y = 0; y < h; ++y) {
            const Pixel* src = pix[y].data();
            uint32_t*    row = dst + y * w;
            for (int x = 0; x < w; ++x) {
                row[x] = ((uint32_t)src[x].a << 24)
                       | ((uint32_t)src[x].r << 16)
                       | ((uint32_t)src[x].g <<  8)
                       |  (uint32_t)src[x].b;
            }
        }
        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
    }
};