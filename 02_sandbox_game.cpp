// ============================================================
//  CGGame 示例：沙盒挖掘游戏
//  以原控制台字符版为基础，改用 CGGame 引擎像素渲染
//  保留原稿：函数名、变量名、掉落物动画、物理、背包
// ============================================================
#include "cggame.h"
#include <map>
#include <cmath>
#include <vector>
using namespace std;

// ============================================================
// config（原 config 段，像素化）
// ============================================================
const int    TILE          = 32;
const int    SCR_W         = 960;
const int    SCR_H         = 640;
const int    MAP_W         = 200;
const int    MAP_H         = 200;
const int    ICO_SIZE      = 24;
const Pixel  BG_COLOR      = rgb(0, 234, 255);
const double PHYSICS_G     = 20.0;
const double HERO_STEP     = 0.2;
const int    HERO_HANDLONG = 5;
const bool   ENABLE_CREATE = false;
const double PI            = 3.14159265358979;

// ============================================================
// 全局
// ============================================================
Screen* g_screen = NULL;
Clock   g_clock;
Font    g_numFont("Consolas", 10);
Font    g_uiFont ("Consolas", 12);

double uptimes  = 0;
double totalTime = 0;

// ============================================================
// structs
// ============================================================
struct BoxType {
    int blood;
    int hard, strength;
    int w, h;
    int z;
};
const int TOTAL_TYPES = 500;
BoxType types[TOTAL_TYPES + 10];

struct Object {
    int    index = 0;
    double x = 0, y = 0;
    int    z = 0;
    int    type = 0;
    Image  img;
    int    w = 1, h = 1;
    double light = 1.0;

    // ---- 掉落物字段 ----
    bool   isFalling         = false;   // 是否为掉落物
    bool   dropSettled       = false;   // 是否已落地静止
    double fallobj_y         = 0;       // 浮点 Y 位置
    double fallobj_vy        = 0;       // 下落速度
    double fallobj_updt      = 0;       // 变成掉落物的时刻（用于动画相位）
    bool   fallobj_firsttouch = true;   // 第一次碰玩家：只变暗，不拾取
};

map<int, Object> obj;
int  obj_tid = 0;
int  blockmap[MAP_H + 5][MAP_W + 5];

struct PosF { double x = 0, y = 0; };

// ============================================================
// hero
// ============================================================
double hero_x = 5;
double hero_y = 0.5;
double hero_vy = 0;
bool   hero_onground = false;
Image  heroImg;

// ============================================================
// pointer
// ============================================================
PosF   pointer_pos;
double pointer_sita = 0;
int    pointer_select = -1;
int    pointer_longclick = 0;

// ============================================================
// bag
// ============================================================
int hero_onhand = 0;
int container[15] = { 0, 1, 2, 3, 4, 5, -1, -1, -1, -1 };
int bag[TOTAL_TYPES + 10] = { 8, 15, 58, 57, 58, 55, 53, 57, 45, 5 };
int bag_space = 27;

// 图像表
Image imgf[10];
Image iconSmall[10];   // 预缩放的掉落物小图标

// ============================================================
// 图像生成（与像素原稿等价）
// ============================================================
Image makeGrassBlock() {
    Image img(TILE, TILE);
    for (int y = 0; y < TILE; ++y)
        for (int x = 0; x < TILE; ++x) {
            int r = 105, g = 70, b = 45;
            if (y > TILE * 0.3) { r = 145; g = 40; b = 40; }
            img.setPixel(x, y, rgb(r, g, b));
        }
    for (int x = 0; x < TILE; ++x)
        for (int y = 0; y < TILE / 3; ++y) {
            int c = 200 + (x + y) % 55;
            img.setPixel(x, y, rgb(c, 255, c));
        }
    for (int x = 0; x < TILE; x += 4)
        img.setPixel(x, TILE / 3, rgb(180, 240, 180));
    return img;
}

Image makeWoodBlock() {
    Image img(TILE, TILE);
    for (int y = 0; y < TILE; ++y)
        for (int x = 0; x < TILE; ++x)
            img.setPixel(x, y, rgb(255, 255, 60));
    for (int i = 0; i < TILE; ++i) {
        img.setPixel(i, 0, rgb(95, 95, 95));
        img.setPixel(i, 2, rgb(95, 95, 95));
        img.setPixel(i, TILE - 1, rgb(95, 95, 95));
        img.setPixel(i, TILE - 3, rgb(95, 95, 95));
    }
    for (int y = 5; y < TILE - 5; ++y)
        for (int x = 4; x < TILE - 4; ++x)
            img.setPixel(x, y, rgb(255, 220, 60));
    for (int y = 8; y < TILE - 8; ++y)
        for (int x = 8; x < TILE - 8; ++x)
            img.setPixel(x, y, rgb(0, 0, 0));
    return img;
}

Image makePickaxe() {
    Image img(TILE, TILE, rgba(0, 0, 0, 0));
    for (int x = 6; x < TILE - 6; ++x) {
        img.setPixel(x, 6, rgb(255, 255, 255));
        img.setPixel(x, 7, rgb(95, 95, 95));
    }
    for (int y = 8; y < TILE - 2; ++y)
        for (int x = TILE / 2 - 2; x < TILE / 2 + 2; ++x)
            img.setPixel(x, y, rgb(255, 255, 0));
    return img;
}

Image makeStone() {
    Image img(TILE, TILE);
    for (int y = 0; y < TILE; ++y)
        for (int x = 0; x < TILE; ++x) {
            int v = 95 + (x * 7 + y * 13) % 40;
            img.setPixel(x, y, rgb(v, v, v + 16));
        }
    for (int i = 0; i < 20; ++i) {
        int x = (i * 37) % TILE;
        int y = (i * 53) % TILE;
        img.setPixel(x, y, rgb(255, 255, 255));
    }
    return img;
}

Image makeGoldOre() {
    Image img = makeStone();
    for (int i = 0; i < 18; ++i) {
        int x = (i * 41) % TILE;
        int y = (i * 29) % TILE;
        for (int dy = 0; dy < 2; ++dy)
            for (int dx = 0; dx < 2; ++dx)
                if (x + dx < TILE && y + dy < TILE)
                    img.setPixel(x + dx, y + dy, rgb(255, 220, 60));
    }
    return img;
}

Image makeHero() {
    const int PW = 32, PH = 64;
    Image img(PW, PH, rgba(0, 0, 0, 0));
    // 头发
    for (int y = 0; y < 6; ++y)
        for (int x = 6; x < 26; ++x) img.setPixel(x, y, rgb(255, 255, 255));
    // 脸
    for (int y = 6; y < 20; ++y)
        for (int x = 6; x < 26; ++x) img.setPixel(x, y, rgb(95, 95, 95));
    // 眼睛
    for (int y = 11; y < 14; ++y) {
        for (int x = 11; x < 14; ++x) img.setPixel(x, y, rgb(0, 0, 0));
        for (int x = 18; x < 21; ++x) img.setPixel(x, y, rgb(0, 0, 0));
    }
    // 身体
    for (int y = 22; y < 44; ++y)
        for (int x = 6; x < 26; ++x) img.setPixel(x, y, rgb(255, 255, 0));
    // 手臂
    for (int y = 24; y < 40; ++y) {
        for (int x = 0; x < 6; ++x)  img.setPixel(x, y, rgb(255, 255, 0));
        for (int x = 26; x < 32; ++x) img.setPixel(x, y, rgb(255, 255, 0));
    }
    // 腿
    for (int y = 44; y < 64; ++y) {
        for (int x = 7; x < 15; ++x)  img.setPixel(x, y, rgb(255, 255, 0));
        for (int x = 17; x < 25; ++x) img.setPixel(x, y, rgb(255, 255, 0));
    }
    // 脚
    for (int y = 62; y < 64; ++y)
        for (int x = 7; x < 25; ++x) img.setPixel(x, y, rgb(255, 255, 255));
    return img;
}

// ============================================================
// 工具函数
// ============================================================
bool checkPosAvail(double px, double py) {
    double pLeft = px, pRight = px + 1.0;
    double pTop = py,  pBottom = py + 2.0;
    if (pLeft < 0.1 || pRight > MAP_W || pTop < 0.1 || pBottom > MAP_H)
        return false;
    int minBX = (int)floor(pLeft), maxBX = (int)floor(pRight);
    int minBY = (int)floor(pTop),  maxBY = (int)floor(pBottom);
    for (int by = minBY; by <= maxBY; ++by)
        for (int bx = minBX; bx <= maxBX; ++bx) {
            if (bx < 0 || bx >= MAP_W || by < 0 || by >= MAP_H) continue;
            int idx = blockmap[by][bx];
            if (idx == -1) continue;
            if (obj[idx].z != 1) continue;
            double bL = bx, bR = bx + 1, bT = by, bB = by + 1;
            bool sep = (pRight <= bL) || (pLeft >= bR) ||
                       (pBottom <= bT) || (pTop >= bB);
            if (!sep) return false;
        }
    return true;
}

double getMouseSita(int mx, int my) {
    double midpx = SCR_W / 2.0, midpy = SCR_H / 2.0;
    double dy = my - midpy, dx = mx - midpx;
    if (dx == 0) {
        if (dy > 0) return -PI / 2;
        if (dy < 0) return PI / 2;
        return 0;
    }
    double sita = atan(dy / dx);
    if (dx < 0) sita += PI;
    return sita;
}

PosF findEmPoint(double sita) {
    double step = 0.1;
    double sx = step * cos(sita), sy = step * sin(sita);
    double tx = hero_x + 0.5, ty = hero_y + 1;
    bool flag = true;
    for (int i = 0; i < HERO_HANDLONG / step; ++i) {
        double nx = tx + sx, ny = ty + sy;
        int bx = (int)floor(nx), by = (int)floor(ny);
        if (bx < 0 || bx >= MAP_W || by < 0 || by >= MAP_H) break;
        if (blockmap[by][bx] != -1) {
            int oid = blockmap[by][bx];
            if (obj.find(oid) != obj.end() && obj[oid].z == 0 && flag) {
                tx = nx; ty = ny; continue;
            }
            if (((int)tx == (int)floor(hero_x) || (int)tx == (int)ceil(hero_x)) &&
                ((int)ty == (int)floor(hero_y) || (int)ty == (int)ceil(hero_y))) break;
            if (((int)tx == (int)floor(hero_x) || (int)tx == (int)ceil(hero_x)) &&
                ((int)ty == (int)floor(hero_y + 1) || (int)ty == (int)ceil(hero_y + 1))) break;
            PosF ret; ret.x = floor(tx); ret.y = floor(ty);
            return ret;
        } else flag = false;
        tx = nx; ty = ny;
    }
    PosF ret; ret.x = -1; ret.y = -1;
    return ret;
}

int findFuPoint(double sita) {
    double step = 0.1;
    double sx = step * cos(sita), sy = step * sin(sita);
    double tx = hero_x + 0.5, ty = hero_y + 1;
    bool flag = true;
    for (int i = 0; i < HERO_HANDLONG / step; ++i) {
        tx += sx; ty += sy;
        int bx = (int)floor(tx), by = (int)floor(ty);
        if (bx < 0 || bx >= MAP_W || by < 0 || by >= MAP_H) break;
        if (blockmap[by][bx] != -1) {
            int oid = blockmap[by][bx];
            if (obj.find(oid) != obj.end() && obj[oid].z == 0 && flag) continue;
            return oid;
        } else flag = false;
    }
    return -1;
}

int createBlock(int x, int y, int type) {
    Object t;
    t.light = 1.0;
    t.img = imgf[type];
    t.w = types[type].w;
    t.h = types[type].h;
    t.type = type;
    t.x = x;
    t.y = y;
    t.z = types[type].z;
    t.fallobj_y = y;
    t.index = obj_tid;
    obj[obj_tid] = t;
    obj_tid++;
    return obj_tid - 1;
}

// ============================================================
// 掉落物（★ 关键：动画 + 正确落地）
// ============================================================
void falldownobj_ontouch(int id) {
    if (obj.find(id) == obj.end()) return;

    if (obj[id].fallobj_firsttouch) {
        // 第一次碰玩家：只变暗
        obj[id].light /= 2;
        obj[id].fallobj_firsttouch = false;
        return;
    }
    // 第二次：拾取
    int tp = obj[id].type;
    if ((bag[tp] > 0 && bag[tp] < 64) || bag_space > 0) {
        bag[tp]++;
        obj.erase(id);
    } else {
        obj[id].light *= 2;
        obj[id].fallobj_firsttouch = true;
    }
}

void falldownobj_update(int id, double dt) {
    if (obj.find(id) == obj.end()) return;
    if (obj[id].dropSettled) return;    // 已落地静止，不再处理

    obj[id].fallobj_vy += PHYSICS_G * dt;
    double nextY = obj[id].fallobj_y + obj[id].fallobj_vy * dt;

    int bx       = (int)floor(obj[id].x);
    int curBy    = (int)floor(obj[id].fallobj_y);
    int targetBy = (int)floor(nextY);

    // ★ 整数行遍历：从下一行到目标行，每行都检查
    for (int by = curBy + 1; by <= targetBy; ++by) {
        if (bx < 0 || bx >= MAP_W || by < 0 || by >= MAP_H) break;

        int hitIdx = blockmap[by][bx];
        if (hitIdx != -1 && hitIdx != id) {
            if (obj.find(hitIdx) == obj.end()) continue;
            if (obj[hitIdx].z != 1) continue;   // 只有实心方块才挡

            // 撞到方块：停在它上方一格
            obj[id].fallobj_y   = by - 1;
            obj[id].y           = by - 1;
            obj[id].fallobj_vy  = 0;
            obj[id].dropSettled = true;         // ★ 落地标记
            return;
        }

        // 碰到玩家
        if ((by == (int)floor(hero_y) || by == (int)floor(hero_y) + 1) &&
            (bx == (int)floor(hero_x) || bx == (int)ceil(hero_x))) {
            falldownobj_ontouch(id);
            return;
        }
    }

    // 未撞到，正常下落
    obj[id].fallobj_y = nextY;
    obj[id].y         = nextY;
}

// ============================================================
// init
// ============================================================
void init() {
    imgf[0] = makeGrassBlock();
    imgf[1] = makeWoodBlock();
    imgf[2] = makePickaxe();
    imgf[3] = makeStone();
    imgf[4] = makeGoldOre();
    imgf[5] = makeHero();
    heroImg = imgf[5];

    // 预缩放掉落物小图标
    for (int i = 0; i < 10; ++i)
        iconSmall[i] = imgf[i].scale(ICO_SIZE, ICO_SIZE);

    types[0].blood = 5; types[0].hard = 5; types[0].strength = 1;
    types[0].w = 1; types[0].h = 1; types[0].z = 1;

    types[1].blood = 5; types[1].hard = 5; types[1].strength = 5;
    types[1].w = 1; types[1].h = 1; types[1].z = 0;

    for (int i = 0; i < 30; ++i) createBlock(i + 1, 10, 0);
    for (int i = 0; i < 30; ++i) createBlock(i + 1, 15, 0);
}

// ============================================================
// update
// ============================================================
void blockmap_update() {
    for (int i = 0; i < MAP_H; ++i)
        for (int j = 0; j < MAP_W; ++j)
            blockmap[i][j] = -1;

    for (map<int, Object>::iterator it = obj.begin(); it != obj.end(); ++it) {
        Object& t = it->second;
        for (int y = (int)t.y; y < (int)t.y + t.h; ++y)
            for (int x = (int)t.x; x < (int)t.x + t.w; ++x) {
                if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) continue;
                blockmap[y][x] = it->first;
            }
    }
}

void runboxfuncs(double dt) {
    vector<int> falling;
    for (map<int, Object>::iterator it = obj.begin(); it != obj.end(); ++it)
        if (it->second.isFalling) falling.push_back(it->first);
    for (size_t i = 0; i < falling.size(); ++i)
        falldownobj_update(falling[i], dt);
}

void runtouchfuncs() {
    double px = hero_x, py = hero_y;
    double pL = px, pR = px + 1, pT = py, pB = py + 2;
    if (pL < 0.1 || pR > MAP_W || pT < 0.1 || pB > MAP_H) return;
    int minX = (int)floor(pL), maxX = (int)floor(pR);
    int minY = (int)floor(pT), maxY = (int)floor(pB);
    for (int by = minY; by <= maxY; ++by)
        for (int bx = minX; bx <= maxX; ++bx) {
            if (bx < 0 || bx >= MAP_W || by < 0 || by >= MAP_H) continue;
            int idx = blockmap[by][bx];
            if (idx == -1) continue;
            if (obj.find(idx) == obj.end()) continue;   // ★ 存在性检查
            if (obj[idx].isFalling) falldownobj_ontouch(idx);
        }
}

// ============================================================
// hero physics
// ============================================================
void update_hero_physics(double dt) {
    if (!hero_onground) hero_vy += PHYSICS_G * dt;
    else hero_vy = 0;

    double nextY = hero_y + hero_vy * dt;
    double step = 0.05;
    if (hero_vy >= 0) {
        for (double y = hero_y; y <= nextY; y += step)
            if (!checkPosAvail(hero_x, y)) {
                hero_y = floor(y); hero_vy = 0; hero_onground = true; return;
            }
    } else {
        for (double y = hero_y; y >= nextY; y -= step)
            if (!checkPosAvail(hero_x, y)) {
                hero_y = ceil(y); hero_vy = 0; return;
            }
    }
    hero_y = nextY;
}

void ground_detect() {
    hero_onground = !checkPosAvail(hero_x, hero_y + 0.1);
}

// ============================================================
// render
// ============================================================
void render_world() {
    g_screen->fill(BG_COLOR);

    double camL = hero_x + 0.5 - SCR_W / 2.0 / TILE;
    double camT = hero_y + 1.0 - SCR_H / 2.0 / TILE;

    int startX = max(0, (int)floor(camL));
    int startY = max(0, (int)floor(camT));
    int endX   = min(MAP_W, (int)ceil(camL + SCR_W / (double)TILE) + 1);
    int endY   = min(MAP_H, (int)ceil(camT + SCR_H / (double)TILE) + 1);

    for (int ty = startY; ty < endY; ++ty) {
        for (int tx = startX; tx < endX; ++tx) {
            int idx = blockmap[ty][tx];
            if (idx == -1) continue;
            if (obj.find(idx) == obj.end()) continue;
            Object& o = obj[idx];
            if (o.isFalling) continue;   // 掉落物单独画

            int sx = (int)((o.x - camL) * TILE);
            int sy = (int)((o.y - camT) * TILE);

            if (o.light < 1.0) {
                Image img = o.img;
                for (int y = 0; y < img.h; ++y)
                    for (int x = 0; x < img.w; ++x) {
                        Pixel& p = img.pix[y][x];
                        if (p.a == 0) continue;
                        p.r = (uint8_t)(p.r * o.light);
                        p.g = (uint8_t)(p.g * o.light);
                        p.b = (uint8_t)(p.b * o.light);
                    }
                g_screen->blit(img, sx, sy);
            } else {
                g_screen->blit(o.img, sx, sy);
            }

            if (idx == pointer_select) {
                g_screen->drawRect(sx, sy, TILE, TILE, rgb(0, 170, 0));
                g_screen->drawRect(sx + 1, sy + 1, TILE - 2, TILE - 2, rgb(0, 170, 0));
            }
        }
    }

    // 放置指示框
    PosF pp = findEmPoint(pointer_sita);
    if (pp.x >= 0) {
        int sx = (int)((pp.x - camL) * TILE);
        int sy = (int)((pp.y - camT) * TILE);
        g_screen->drawRect(sx, sy, TILE, TILE, rgb(153, 153, 153));
    }

    // ---- 掉落物（★ 上下浮动动画）----
    for (map<int, Object>::iterator it = obj.begin(); it != obj.end(); ++it) {
        Object& o = it->second;
        if (!o.isFalling) continue;
        if (obj.find(it->first) == obj.end()) continue;

        // 格子左上角的屏幕坐标
        int baseX = (int)((o.x - camL) * TILE) + (TILE - ICO_SIZE) / 2;
        int baseY = (int)((o.y - camT) * TILE) + (TILE - ICO_SIZE);   // 底部对齐

        // ★ 上下浮动：基于 (uptimes - fallobj_updt)，与原稿一致
        double phase = (uptimes - o.fallobj_updt) * 0.1;
        int bob = (int)(fabs(sin(phase)) * (TILE - ICO_SIZE) * 0.5);

        Image icon = iconSmall[o.type];
        if (o.light < 1.0) {
            icon = iconSmall[o.type];
            for (int y = 0; y < icon.h; ++y)
                for (int x = 0; x < icon.w; ++x) {
                    Pixel& p = icon.pix[y][x];
                    if (p.a == 0) continue;
                    p.r = (uint8_t)(p.r * o.light);
                    p.g = (uint8_t)(p.g * o.light);
                    p.b = (uint8_t)(p.b * o.light);
                }
        }
        g_screen->blit(icon, baseX, baseY - bob);
    }

    // ---- 英雄 ----
    int hx = (int)((hero_x - camL) * TILE);
    int hy = (int)((hero_y - camT) * TILE);
    g_screen->blit(heroImg, hx, hy);
}

void render_hotbar() {
    int slotW = ICO_SIZE + 8;
    int barW = 9 * (slotW + 2);
    int barX = (SCR_W - barW) / 2;
    int barY = SCR_H - slotW - 16;

    g_screen->fillRect(barX - 4, barY - 4, barW + 8, slotW + 12, rgba(0, 0, 0, 180));
    g_screen->drawRect(barX - 4, barY - 4, barW + 8, slotW + 12, rgb(255, 192, 0));

    for (int i = 0; i < 9; ++i) {
        int sx = barX + i * (slotW + 2) + 2;
        int sy = barY;

        if (container[i] == -1) {
            g_screen->fillRect(sx, sy, slotW, slotW, rgb(255, 229, 10));
        } else {
            Image small = iconSmall[container[i]];
            g_screen->blit(small, sx + (slotW - ICO_SIZE) / 2,
                                 sy + (slotW - ICO_SIZE) / 2);
        }
        if (i == hero_onhand) {
            g_screen->drawRect(sx, sy, slotW, slotW, rgb(0, 170, 0));
            g_screen->drawRect(sx + 1, sy + 1, slotW - 2, slotW - 2, rgb(0, 170, 0));
        }
        if (container[i] != -1 && bag[container[i]] > 0) {
            g_screen->drawText(to_string(bag[container[i]]),
                               sx + slotW - 14, sy + slotW - 14,
                               rgb(0, 0, 255), g_numFont);
        }
    }
}

// ============================================================
// main
// ============================================================
int main() {
    srand((unsigned)time(NULL));

    Screen screen(SCR_W, SCR_H, BG_COLOR);
    g_screen = &screen;

    init();

    bool running = true;

    while (running) {
        double dt = g_clock.tick(60);
        if (dt > 0.1) dt = 0.1;
        uptimes++;
        totalTime += dt;

        // ---- 事件 ----
        const vector<Event>& events = screen.eventGet();
        for (size_t i = 0; i < events.size(); ++i) {
            const Event& e = events[i];
            if (e.type == EVENT_QUIT) running = false;
            if (e.type == EVENT_KEYDOWN && e.key == Key::ESCAPE) running = false;
            if (e.type == EVENT_KEYDOWN) {
                for (int k = 0; k < 9; ++k)
                    if (e.key == '1' + k) hero_onhand = k;
            }
        }

        // ---- 移动 ----
        if (screen.keyPressed(Key::A) || screen.keyPressed(Key::LEFT)) {
            double nx = hero_x - HERO_STEP;
            if (checkPosAvail(nx, hero_y)) hero_x = nx;
        }
        if (screen.keyPressed(Key::D) || screen.keyPressed(Key::RIGHT)) {
            double nx = hero_x + HERO_STEP;
            if (checkPosAvail(nx, hero_y)) hero_x = nx;
        }

        // ---- 跳跃 ----
        ground_detect();
        if ((screen.keyPressed(Key::W) || screen.keyPressed(Key::SPACE) ||
             screen.keyPressed(Key::UP)) && (hero_onground || ENABLE_CREATE)) {
            hero_vy = -9;
            hero_onground = false;
        }

        // ---- 指针 ----
        pointer_sita = getMouseSita(screen.mouseX(), screen.mouseY());
        pointer_pos  = findEmPoint(pointer_sita);

        // ---- 放置（中键 / Alt）----
        if (screen.mousePressed(MouseButton::MIDDLE) || screen.keyPressed(Key::ALT)) {
            if (pointer_pos.x != -1 && container[hero_onhand] != -1) {
                createBlock((int)pointer_pos.x, (int)pointer_pos.y,
                            container[hero_onhand]);
            }
            pointer_select = -1;
        }

        // ---- 挖掘（左键 / Ctrl）----
        if (screen.mousePressed(MouseButton::LEFT) || screen.keyPressed(Key::CTRL)) {
            int tsl = findFuPoint(pointer_sita);
            if (pointer_select != tsl) {
                pointer_select = tsl;
                pointer_longclick = 0;
            } else if (tsl != -1 && obj.find(tsl) != obj.end()) {
                pointer_longclick++;
                int handType = container[hero_onhand];
                if (handType >= 0 &&
                    pointer_longclick * types[handType].strength >=
                    types[obj[tsl].type].blood) {
                    // ★ 变成掉落物
                    obj[tsl].isFalling          = true;
                    obj[tsl].dropSettled        = false;
                    obj[tsl].fallobj_vy         = 0;
                    obj[tsl].fallobj_y          = obj[tsl].y;
                    obj[tsl].fallobj_updt       = uptimes;   // 动画相位起点
                    obj[tsl].fallobj_firsttouch = true;
                    obj[tsl].z                  = 0;
                }
            }
        }

        // ---- 更新 ----
        blockmap_update();
        runboxfuncs(dt);
        runtouchfuncs();
        blockmap_update();
        update_hero_physics(dt);

        // ---- 渲染 ----
        render_world();
        render_hotbar();
        screen.update();
    }

    return 0;
}