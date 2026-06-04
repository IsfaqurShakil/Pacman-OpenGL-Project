#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <string>
#include <sstream>
#include <algorithm>

const int WIN_W = 760;
const int WIN_H = 820;
const int ROWS = 23;
const int COLS = 21;
const int CELL = 32;
const int OX = (WIN_W - COLS * CELL) / 2;
const int OY = 30;
const float PI = 3.14159265f;

const int RIGHT = 0;
const int LEFT  = 1;
const int UP    = 2;
const int DOWN  = 3;

int DR[4] = {0, 0, -1, 1};
int DC[4] = {1, -1, 0, 0};

int maze[ROWS][COLS] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,3,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,3,1},
    {1,0,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,0,1},
    {1,0,1,1,0,1,1,1,0,0,1,0,0,1,1,1,0,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,0,1,1,0,1,1,1,1,1,0,1,1,0,1,1,1,1},
    {1,1,1,1,0,1,1,0,0,0,1,0,0,0,1,1,0,1,1,1,1},
    {1,1,1,1,0,1,1,1,1,2,2,2,1,1,1,1,0,1,1,1,1},
    {1,1,1,1,0,1,2,2,2,2,2,2,2,2,2,1,0,1,1,1,1},
    {1,1,1,1,0,1,2,1,1,4,4,4,1,1,2,1,0,1,1,1,1},
    {2,2,2,2,0,2,2,1,4,4,4,4,4,1,2,2,0,2,2,2,2},
    {1,1,1,1,0,1,2,1,1,1,1,1,1,1,2,1,0,1,1,1,1},
    {1,1,1,1,0,1,2,2,2,2,2,2,2,2,2,1,0,1,1,1,1},
    {1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1},
    {1,3,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,3,1},
    {1,0,1,1,0,1,1,1,1,0,1,0,1,1,1,1,0,1,1,0,1},
    {1,0,0,1,0,0,0,0,0,0,2,0,0,0,0,0,0,1,0,0,1},
    {1,1,0,1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,0,1,1},
    {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,3,0,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,0,3,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

int originalMaze[ROWS][COLS];

enum GameState { MENU, PLAYING, PAUSED, GAME_OVER, WIN, HELP };
GameState gameState = MENU;
GameState previousState = MENU;

struct Player {
    float x, y;
    int row, col;
    int dir, nextDir;
    float speed;
    float mouthAngle;
    bool mouthOpening;
    bool powerMode;
    int powerTimer;
};

struct Ghost {
    int id;
    float x, y;
    int row, col;
    int dir;
    float speed;
    float r, g, b;
    bool scared;
    bool alive;
    int scaredTimer;
    bool fireActive;
    float fireX, fireY, fireVX, fireVY;
    int fireCooldown;
    bool cloneActive;
    float cloneX, cloneY;
    int cloneRow, cloneCol, cloneDir;
    int cloneTimer;
};

Player pac;
Ghost ghosts[4];

int score = 0;
int highScore = 0;
int lives = 3;
int levelNo = 1;
int elapsedMs = 0;
int menuChoice = 0;
bool helpFromMenu = true;

void spawnGhost(int id);

float cellToPixX(int col) {
    return OX + col * CELL + CELL / 2.0f;
}

float cellToPixY(int row) {
    return WIN_H - (OY + row * CELL + CELL / 2.0f);
}

int clampInt(int value, int low, int high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

int wrapCol(int col) {
    if (col < 0) return COLS - 1;
    if (col >= COLS) return 0;
    return col;
}

bool isWall(int row, int col) {
    if (row < 0 || row >= ROWS) return true;
    col = wrapCol(col);
    return maze[row][col] == 1;
}

bool canMoveDir(int row, int col, int dir) {
    int nr = row + DR[dir];
    int nc = wrapCol(col + DC[dir]);
    if (nr < 0 || nr >= ROWS) return false;
    return maze[nr][nc] != 1;
}

bool nearCenter(float x, float y, int row, int col) {
    return std::fabs(x - cellToPixX(col)) <= 1.2f && std::fabs(y - cellToPixY(row)) <= 1.2f;
}

void setPixel(int x, int y) {
    glVertex2i(x, y);
}

void drawLineBresenham(int x1, int y1, int x2, int y2) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    glBegin(GL_POINTS);
    while (true) {
        setPixel(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
    glEnd();
}

void drawLineDDA(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float steps = (std::fabs(dx) > std::fabs(dy)) ? std::fabs(dx) : std::fabs(dy);
    if (steps == 0.0f) {
        glBegin(GL_POINTS);
        setPixel((int)x1, (int)y1);
        glEnd();
        return;
    }
    float xInc = dx / steps;
    float yInc = dy / steps;
    float x = x1;
    float y = y1;

    glBegin(GL_POINTS);
    for (int i = 0; i <= (int)steps; i++) {
        setPixel((int)std::round(x), (int)std::round(y));
        x += xInc;
        y += yInc;
    }
    glEnd();
}

void plotMidpointOctants(int x, int y, int cx, int cy) {
    setPixel(cx + x, cy + y);
    setPixel(cx - x, cy + y);
    setPixel(cx + x, cy - y);
    setPixel(cx - x, cy - y);
    setPixel(cx + y, cy + x);
    setPixel(cx - y, cy + x);
    setPixel(cx + y, cy - x);
    setPixel(cx - y, cy - x);
}

void drawCircleMidpoint(int cx, int cy, int radius) {
    int x = 0;
    int y = radius;
    int d = 1 - radius;

    glBegin(GL_POINTS);
    plotMidpointOctants(x, y, cx, cy);
    while (y > x) {
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
        plotMidpointOctants(x, y, cx, cy);
    }
    glEnd();
}

void drawFilledCircle(float cx, float cy, float radius) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 60; i++) {
        float a = 2.0f * PI * i / 60.0f;
        glVertex2f(cx + std::cos(a) * radius, cy + std::sin(a) * radius);
    }
    glEnd();
}

void drawRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawText(float x, float y, const std::string& text, void* font = GLUT_BITMAP_8_BY_13) {
    glRasterPos2f(x, y);
    for (size_t i = 0; i < text.size(); i++) {
        glutBitmapCharacter(font, text[i]);
    }
}

void drawTextCenter(float y, const std::string& text, void* font = GLUT_BITMAP_HELVETICA_18) {
    int width = 0;
    for (size_t i = 0; i < text.size(); i++) width += glutBitmapWidth(font, text[i]);
    drawText((WIN_W - width) / 2.0f, y, text, font);
}

std::string intToString(int n) {
    std::stringstream ss;
    ss << n;
    return ss.str();
}

std::string timeString() {
    std::stringstream ss;
    ss << (elapsedMs / 1000) << "s";
    return ss.str();
}

int oppositeDir(int d) {
    if (d == RIGHT) return LEFT;
    if (d == LEFT) return RIGHT;
    if (d == UP) return DOWN;
    return UP;
}

int randomDir(int row, int col, int oldDir) {
    int dirs[4];
    int count = 0;
    for (int d = 0; d < 4; d++) {
        if (d == oppositeDir(oldDir) && count > 0) continue;
        if (canMoveDir(row, col, d)) dirs[count++] = d;
    }
    if (count == 0) {
        if (canMoveDir(row, col, oppositeDir(oldDir))) return oppositeDir(oldDir);
        return oldDir;
    }
    return dirs[std::rand() % count];
}

int chaseDir(Ghost& gh, int targetRow, int targetCol) {
    int best = gh.dir;
    float bestDist = 1e9f;
    int found = 0;

    for (int d = 0; d < 4; d++) {
        if (d == oppositeDir(gh.dir)) continue;
        if (!canMoveDir(gh.row, gh.col, d)) continue;

        int nr = gh.row + DR[d];
        int nc = wrapCol(gh.col + DC[d]);
        float dist = (float)((nr - targetRow) * (nr - targetRow) + (nc - targetCol) * (nc - targetCol));
        if (dist < bestDist) {
            bestDist = dist;
            best = d;
            found = 1;
        }
    }

    if (!found) {
        for (int d = 0; d < 4; d++) {
            if (canMoveDir(gh.row, gh.col, d)) return d;
        }
    }
    return best;
}

void loseLife() {
    lives--;
    if (score > highScore) highScore = score;

    if (lives <= 0) {
        gameState = GAME_OVER;
        return;
    }

    pac.row = 20;
    pac.col = 10;
    pac.x = cellToPixX(pac.col);
    pac.y = cellToPixY(pac.row);
    pac.dir = LEFT;
    pac.nextDir = LEFT;
    pac.powerMode = false;
    pac.powerTimer = 0;

    for (int i = 0; i < 4; i++) spawnGhost(i);
}

void spawnGhost(int id) {
    Ghost& gh = ghosts[id];
    gh.id = id;
    gh.row = 10;
    gh.col = 9 + id;
    if (id == 3) gh.col = 11;
    gh.x = cellToPixX(gh.col);
    gh.y = cellToPixY(gh.row);
    gh.dir = (id % 2 == 0) ? LEFT : RIGHT;
    gh.speed = std::min(4.0f, 1.45f + id * 0.17f + (levelNo - 1) * 0.15f);
    gh.scared = false;
    gh.alive = true;
    gh.scaredTimer = 0;
    gh.fireActive = false;
    gh.fireCooldown = 160 + std::rand() % 200;
    gh.cloneActive = false;
    gh.cloneTimer = 250 + std::rand() % 220;

    if (id == 0) { gh.r = 1.0f; gh.g = 0.0f; gh.b = 0.0f; }
    if (id == 1) { gh.r = 1.0f; gh.g = 0.45f; gh.b = 0.78f; }
    if (id == 2) { gh.r = 0.0f; gh.g = 0.95f; gh.b = 1.0f; }
    if (id == 3) { gh.r = 1.0f; gh.g = 0.55f; gh.b = 0.0f; }
}

void resetMaze() {
    std::memcpy(maze, originalMaze, sizeof(maze));
}

void resetGame(bool fullReset) {
    if (fullReset) {
        score = 0;
        lives = 3;
        levelNo = 1;
        elapsedMs = 0;
    }

    resetMaze();

    pac.row = 20;
    pac.col = 10;
    pac.x = cellToPixX(pac.col);
    pac.y = cellToPixY(pac.row);
    pac.dir = LEFT;
    pac.nextDir = LEFT;
    pac.speed = 2.0f;
    pac.mouthAngle = 0.35f;
    pac.mouthOpening = true;
    pac.powerMode = false;
    pac.powerTimer = 0;

    for (int i = 0; i < 4; i++) spawnGhost(i);
    gameState = PLAYING;
}

void drawMaze() {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            float px = OX + c * CELL;
            float py = WIN_H - (OY + (r + 1) * CELL);
            int value = maze[r][c];

            if (value == 1) {
                glColor3f(0.05f, 0.05f, 0.48f);
                drawRect(px + 1, py + 1, CELL - 2, CELL - 2);
                glColor3f(0.18f, 0.18f, 0.95f);
                glPointSize(1.5f);
                int bx0 = (int)px + 4;
                int by0 = (int)py + 4;
                int bx1 = (int)px + CELL - 4;
                int by1 = (int)py + CELL - 4;
                drawLineBresenham(bx0, by0, bx1, by0);
                drawLineBresenham(bx1, by0, bx1, by1);
                drawLineBresenham(bx1, by1, bx0, by1);
                drawLineBresenham(bx0, by1, bx0, by0);
            } else if (value == 4) {
                glColor4f(0.22f, 0.04f, 0.18f, 0.65f);
                drawRect(px + 1, py + 1, CELL - 2, CELL - 2);
            } else if (value == 0) {
                glColor3f(1.0f, 1.0f, 0.78f);
                glPointSize(2.0f);
                drawCircleMidpoint((int)cellToPixX(c), (int)cellToPixY(r), 3);
            } else if (value == 3) {
                if ((elapsedMs / 300) % 2 == 0) {
                    glColor3f(1.0f, 1.0f, 1.0f);
                    drawFilledCircle(cellToPixX(c), cellToPixY(r), 6.0f);
                    glColor3f(0.8f, 0.8f, 0.0f);
                    glPointSize(1.5f);
                    drawCircleMidpoint((int)cellToPixX(c), (int)cellToPixY(r), 6);
                }
            }
        }
    }
}

void drawPacmanAt(float cx, float cy, int dir, float radius) {
    float baseAngleDeg[4] = {0.0f, 180.0f, 90.0f, 270.0f};
    float ba = baseAngleDeg[dir] * PI / 180.0f;
    float mouth = pac.mouthAngle;

    glColor3f(1.0f, 0.85f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 50; i++) {
        float a = ba + mouth + (2.0f * PI - 2.0f * mouth) * i / 50.0f;
        glVertex2f(cx + std::cos(a) * radius, cy + std::sin(a) * radius);
    }
    glEnd();

    float eyeA = ba + PI / 2.8f;
    glColor3f(0.0f, 0.0f, 0.0f);
    drawFilledCircle(cx + std::cos(eyeA) * radius * 0.45f, cy + std::sin(eyeA) * radius * 0.45f, radius * 0.12f);
}

void drawPacman() {
    drawPacmanAt(pac.x, pac.y, pac.dir, CELL / 2.0f - 3.0f);
}

void drawGhostShape(float cx, float cy, float rr, float gg, float bb, bool scared, float shade) {
    float radius = CELL / 2.0f - 4.0f;
    float left = cx - radius;
    float bottom = cy - radius;

    if (scared) glColor3f(0.05f * shade, 0.15f * shade, 1.0f * shade);
    else glColor3f(rr * shade, gg * shade, bb * shade);

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy + 2);
    for (int i = 0; i <= 30; i++) {
        float a = PI * i / 30.0f;
        glVertex2f(cx + std::cos(a) * radius, cy + 2 + std::sin(a) * radius);
    }
    glEnd();

    drawRect(left, bottom, radius * 2.0f, radius + 3.0f);

    glBegin(GL_TRIANGLES);
    glVertex2f(left, bottom);
    glVertex2f(left + radius * 0.35f, bottom - 6);
    glVertex2f(left + radius * 0.7f, bottom);
    glVertex2f(left + radius * 0.7f, bottom);
    glVertex2f(left + radius, bottom - 6);
    glVertex2f(left + radius * 1.3f, bottom);
    glVertex2f(left + radius * 1.3f, bottom);
    glVertex2f(left + radius * 1.65f, bottom - 6);
    glVertex2f(left + radius * 2.0f, bottom);
    glEnd();

    if (!scared) {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawFilledCircle(cx - radius * 0.35f, cy + radius * 0.15f, 4.5f);
        drawFilledCircle(cx + radius * 0.35f, cy + radius * 0.15f, 4.5f);
        glColor3f(0.0f, 0.0f, 0.0f);
        drawFilledCircle(cx - radius * 0.35f + 1.5f, cy + radius * 0.15f, 2.0f);
        drawFilledCircle(cx + radius * 0.35f + 1.5f, cy + radius * 0.15f, 2.0f);
    } else {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawFilledCircle(cx - radius * 0.35f, cy + radius * 0.2f, 2.0f);
        drawFilledCircle(cx + radius * 0.35f, cy + radius * 0.2f, 2.0f);
        glPointSize(1.5f);
        for (int i = 0; i < 6; i++) {
            float mx0 = cx - radius * 0.6f + i * (radius * 1.2f / 6.0f);
            float my0 = cy - radius * 0.35f + ((i % 2 == 0) ? 0.0f : -4.0f);
            float mx1 = cx - radius * 0.6f + (i + 1) * (radius * 1.2f / 6.0f);
            float my1 = cy - radius * 0.35f + ((i % 2 == 0) ? -4.0f : 0.0f);
            drawLineDDA(mx0, my0, mx1, my1);
        }
    }

    glColor3f(0.0f, 0.0f, 0.0f);
    glPointSize(1.0f);
    drawCircleMidpoint((int)cx, (int)(cy + 2), (int)radius);
}

void drawGhost(Ghost& gh) {
    if (!gh.alive) return;
    drawGhostShape(gh.x, gh.y, gh.r, gh.g, gh.b, gh.scared, 1.0f);

    if (gh.id == 0 && gh.fireActive) {
        glColor3f(1.0f, 0.35f, 0.0f);
        drawFilledCircle(gh.fireX, gh.fireY, 7.0f);
        glColor3f(1.0f, 1.0f, 0.0f);
        drawFilledCircle(gh.fireX + 2.0f, gh.fireY + 1.0f, 3.5f);
    }

    if (gh.id == 3 && gh.cloneActive) {
        drawGhostShape(gh.cloneX, gh.cloneY, gh.r, gh.g, gh.b, gh.scared, 0.45f);
    }
}

void drawHUD() {
    glColor3f(0.0f, 0.0f, 0.0f);
    drawRect(0, 0, WIN_W, 60);

    glColor3f(1.0f, 1.0f, 0.0f);
    drawText(30, 28, "SCORE: " + intToString(score), GLUT_BITMAP_HELVETICA_18);
    drawText(190, 28, "HI: " + intToString(highScore), GLUT_BITMAP_HELVETICA_18);
    drawText(365, 28, "LEVEL: " + intToString(levelNo), GLUT_BITMAP_HELVETICA_18);
    drawText(510, 28, "TIME: " + timeString(), GLUT_BITMAP_HELVETICA_18);

    for (int i = 0; i < lives; i++) {
        float x = 660 + i * 28;
        float y = 32;
        float oldMouth = pac.mouthAngle;
        pac.mouthAngle = 0.45f;
        drawPacmanAt(x, y, RIGHT, 10.0f);
        pac.mouthAngle = oldMouth;
    }

    if (pac.powerMode) {
        glColor3f(0.4f, 0.8f, 1.0f);
        drawText(300, 8, "POWER MODE", GLUT_BITMAP_8_BY_13);
    }
}

void drawMenu() {
    glColor3f(0.02f, 0.02f, 0.08f);
    drawRect(0, 0, WIN_W, WIN_H);

    glColor3f(1.0f, 0.85f, 0.0f);
    drawTextCenter(630, "PAC-MAN", GLUT_BITMAP_TIMES_ROMAN_24);

    float oldMouth = pac.mouthAngle;
    pac.mouthAngle = 0.45f + 0.20f * std::sin(elapsedMs / 160.0f);
    drawPacmanAt(WIN_W / 2.0f - 80.0f, 550.0f, RIGHT, 24.0f);
    pac.mouthAngle = oldMouth;

    drawGhostShape(WIN_W / 2.0f - 20.0f, 550.0f, 1.0f, 0.0f, 0.0f, false, 1.0f);
    drawGhostShape(WIN_W / 2.0f + 30.0f, 550.0f, 1.0f, 0.45f, 0.78f, false, 1.0f);
    drawGhostShape(WIN_W / 2.0f + 80.0f, 550.0f, 0.0f, 0.95f, 1.0f, false, 1.0f);

    std::string items[3] = {"START GAME", "HELP", "EXIT"};
    for (int i = 0; i < 3; i++) {
        if (i == menuChoice) glColor3f(1.0f, 1.0f, 0.0f);
        else glColor3f(0.75f, 0.75f, 0.75f);
        drawTextCenter(420 - i * 45, (i == menuChoice ? "> " : "  ") + items[i], GLUT_BITMAP_HELVETICA_18);
    }

    glColor3f(0.5f, 1.0f, 1.0f);
    drawTextCenter(210, "Use UP/DOWN and ENTER", GLUT_BITMAP_8_BY_13);
    drawTextCenter(185, "CSE 426 Computer Graphics Lab", GLUT_BITMAP_8_BY_13);
}

void drawHelp() {
    glColor3f(0.01f, 0.01f, 0.06f);
    drawRect(0, 0, WIN_W, WIN_H);

    glColor3f(1.0f, 0.78f, 0.25f);
    drawTextCenter(720, "HELP", GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0.8f, 0.8f, 0.25f);
    glPointSize(1.5f);
    drawLineDDA(140, 700, 620, 700);

    float y = 660;
    glColor3f(1.0f, 1.0f, 0.0f);
    drawText(150, y, "CONTROLS", GLUT_BITMAP_HELVETICA_18); y -= 35;
    glColor3f(0.85f, 0.85f, 0.85f);
    drawText(150, y, "Arrow Keys : Move Pac-Man"); y -= 25;
    drawText(150, y, "P : Pause / Resume"); y -= 25;
    drawText(150, y, "ESC : Back to Menu"); y -= 45;

    glColor3f(1.0f, 1.0f, 0.0f);
    drawText(150, y, "OBJECTIVE", GLUT_BITMAP_HELVETICA_18); y -= 35;
    glColor3f(0.85f, 0.85f, 0.85f);
    drawText(150, y, "Eat all dots to win. Avoid ghosts or lose a life."); y -= 25;
    drawText(150, y, "Large dots activate power mode and scared ghosts."); y -= 45;

    glColor3f(1.0f, 1.0f, 0.0f);
    drawText(150, y, "GHOSTS", GLUT_BITMAP_HELVETICA_18); y -= 35;
    glColor3f(1.0f, 0.2f, 0.2f); drawText(150, y, "Blinky (red)    : chases and shoots fire"); y -= 25;
    glColor3f(1.0f, 0.45f, 0.78f); drawText(150, y, "Pinky (pink)    : predicts your direction"); y -= 25;
    glColor3f(0.0f, 0.95f, 1.0f); drawText(150, y, "Inky (cyan)     : flanks using Blinky position"); y -= 25;
    glColor3f(1.0f, 0.55f, 0.0f); drawText(150, y, "Clyde (orange)  : runs away and creates clone"); y -= 45;

    glColor3f(0.55f, 1.0f, 1.0f);
    drawTextCenter(120, "Press ESC to go back", GLUT_BITMAP_HELVETICA_18);
}

void drawPauseOverlay() {
    drawMaze();
    drawPacman();
    for (int i = 0; i < 4; i++) drawGhost(ghosts[i]);
    drawHUD();

    glColor4f(0.0f, 0.0f, 0.0f, 0.65f);
    drawRect(0, 0, WIN_W, WIN_H);
    glColor3f(1.0f, 1.0f, 0.0f);
    drawTextCenter(430, "PAUSED", GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0.8f, 0.9f, 1.0f);
    drawTextCenter(390, "Press P to Resume", GLUT_BITMAP_HELVETICA_18);
}

void drawEndScreen(bool win) {
    glColor3f(0.0f, 0.0f, 0.0f);
    drawRect(0, 0, WIN_W, WIN_H);

    if (win) glColor3f(0.0f, 1.0f, 0.6f);
    else glColor3f(1.0f, 0.25f, 0.25f);
    drawTextCenter(470, win ? "YOU WIN!" : "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(1.0f, 1.0f, 0.7f);
    drawTextCenter(420, "Score: " + intToString(score), GLUT_BITMAP_HELVETICA_18);
    drawTextCenter(385, "Time: " + intToString(elapsedMs / 1000) + " seconds", GLUT_BITMAP_HELVETICA_18);
    glColor3f(0.4f, 1.0f, 1.0f);
    drawTextCenter(350, "High Score: " + intToString(highScore), GLUT_BITMAP_HELVETICA_18);
    drawTextCenter(300, "Press M for Menu | Press R to Replay", GLUT_BITMAP_HELVETICA_18);
}

void renderGame() {
    drawMaze();
    drawPacman();
    for (int i = 0; i < 4; i++) drawGhost(ghosts[i]);
    drawHUD();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (gameState == MENU) drawMenu();
    else if (gameState == HELP) drawHelp();
    else if (gameState == PLAYING) renderGame();
    else if (gameState == PAUSED) drawPauseOverlay();
    else if (gameState == GAME_OVER) drawEndScreen(false);
    else if (gameState == WIN) drawEndScreen(true);

    glutSwapBuffers();
}

void eatDotIfAny() {
    if (maze[pac.row][pac.col] == 0) {
        maze[pac.row][pac.col] = 2;
        score += 10;
    } else if (maze[pac.row][pac.col] == 3) {
        maze[pac.row][pac.col] = 2;
        score += 50;
        pac.powerMode = true;
        pac.powerTimer = 300;
        for (int i = 0; i < 4; i++) {
            if (ghosts[i].alive) {
                ghosts[i].scared = true;
                ghosts[i].scaredTimer = 300;
            }
        }
    }
}

bool allDotsEaten() {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (maze[r][c] == 0 || maze[r][c] == 3) return false;
        }
    }
    return true;
}

void updatePacman() {
    if (pac.mouthOpening) {
        pac.mouthAngle += 0.05f;
        if (pac.mouthAngle > 0.65f) pac.mouthOpening = false;
    } else {
        pac.mouthAngle -= 0.05f;
        if (pac.mouthAngle < 0.12f) pac.mouthOpening = true;
    }

    if (pac.powerMode) {
        pac.powerTimer--;
        if (pac.powerTimer <= 0) {
            pac.powerMode = false;
            for (int i = 0; i < 4; i++) ghosts[i].scared = false;
        }
    }

    if (nearCenter(pac.x, pac.y, pac.row, pac.col)) {
        pac.x = cellToPixX(pac.col);
        pac.y = cellToPixY(pac.row);
        eatDotIfAny();

        if (allDotsEaten()) {
            levelNo++;
            if (score > highScore) highScore = score;
            if (levelNo > 3) {
                gameState = WIN;
                return;
            }
            resetGame(false);
            return;
        }

        if (canMoveDir(pac.row, pac.col, pac.nextDir)) pac.dir = pac.nextDir;
        if (!canMoveDir(pac.row, pac.col, pac.dir)) return;
    }

    int tr = pac.row + DR[pac.dir];
    int tc = pac.col + DC[pac.dir];

    if (tc < 0) {
        pac.col = COLS - 1;
        pac.x = cellToPixX(pac.col);
        return;
    }
    if (tc >= COLS) {
        pac.col = 0;
        pac.x = cellToPixX(pac.col);
        return;
    }

    if (tr < 0 || tr >= ROWS || isWall(tr, tc)) return;

    float tx = cellToPixX(tc);
    float ty = cellToPixY(tr);
    float vx = (tx > pac.x) ? pac.speed : ((tx < pac.x) ? -pac.speed : 0.0f);
    float vy = (ty > pac.y) ? pac.speed : ((ty < pac.y) ? -pac.speed : 0.0f);

    if (std::fabs(tx - pac.x) <= std::fabs(vx) && std::fabs(ty - pac.y) <= std::fabs(vy)) {
        pac.x = tx;
        pac.y = ty;
        pac.row = tr;
        pac.col = tc;
    } else {
        pac.x += vx;
        pac.y += vy;
    }
}

void moveGhostBody(Ghost& gh) {
    if (nearCenter(gh.x, gh.y, gh.row, gh.col)) {
        gh.x = cellToPixX(gh.col);
        gh.y = cellToPixY(gh.row);

        if (gh.scared) {
            gh.dir = randomDir(gh.row, gh.col, gh.dir);
        } else {
            int targetRow = pac.row;
            int targetCol = pac.col;

            if (gh.id == 1) {
                targetRow = clampInt(pac.row + DR[pac.dir] * 4, 1, ROWS - 2);
                targetCol = wrapCol(pac.col + DC[pac.dir] * 4);
            } else if (gh.id == 2) {
                int aheadRow = clampInt(pac.row + DR[pac.dir] * 2, 1, ROWS - 2);
                int aheadCol = wrapCol(pac.col + DC[pac.dir] * 2);
                targetRow = clampInt(aheadRow + (aheadRow - ghosts[0].row), 1, ROWS - 2);
                targetCol = wrapCol(aheadCol + (aheadCol - ghosts[0].col));
            } else if (gh.id == 3) {
                int dist = std::abs(gh.row - pac.row) + std::abs(gh.col - pac.col);
                if (dist <= 6) {
                    targetRow = ROWS - 2;
                    targetCol = 1;
                }
            }
            gh.dir = chaseDir(gh, targetRow, targetCol);
        }
    }

    if (!canMoveDir(gh.row, gh.col, gh.dir)) return;

    int tr = gh.row + DR[gh.dir];
    int tc = wrapCol(gh.col + DC[gh.dir]);
    float tx = cellToPixX(tc);
    float ty = cellToPixY(tr);
    float sp = gh.scared ? gh.speed * 0.55f : gh.speed;
    float vx = (tx > gh.x) ? sp : ((tx < gh.x) ? -sp : 0.0f);
    float vy = (ty > gh.y) ? sp : ((ty < gh.y) ? -sp : 0.0f);

    if (std::fabs(tx - gh.x) <= std::fabs(vx) && std::fabs(ty - gh.y) <= std::fabs(vy)) {
        gh.x = tx;
        gh.y = ty;
        gh.row = tr;
        gh.col = tc;
    } else {
        gh.x += vx;
        gh.y += vy;
    }
}

void updateBlinkyFire(Ghost& gh) {
    if (gh.id != 0 || !gh.alive) return;

    if (!gh.fireActive) {
        gh.fireCooldown--;
        if (gh.fireCooldown <= 0) {
            float dx = pac.x - gh.x;
            float dy = pac.y - gh.y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len < 1.0f) len = 1.0f;
            gh.fireX = gh.x;
            gh.fireY = gh.y;
            gh.fireVX = dx / len * 4.6f;
            gh.fireVY = dy / len * 4.6f;
            gh.fireActive = true;
            gh.fireCooldown = 250 + std::rand() % 250;
        }
    } else {
        gh.fireX += gh.fireVX;
        gh.fireY += gh.fireVY;
        if (gh.fireX < 0 || gh.fireX > WIN_W || gh.fireY < 0 || gh.fireY > WIN_H) {
            gh.fireActive = false;
        }
        float dx = pac.x - gh.fireX;
        float dy = pac.y - gh.fireY;
        if (std::sqrt(dx * dx + dy * dy) < CELL * 0.45f) {
            gh.fireActive = false;
            loseLife();
        }
    }
}

void updateClydeClone(Ghost& gh) {
    if (gh.id != 3 || !gh.alive) return;

    if (!gh.cloneActive) {
        gh.cloneTimer--;
        if (gh.cloneTimer <= 0) {
            gh.cloneActive = true;
            gh.cloneX = gh.x;
            gh.cloneY = gh.y;
            gh.cloneRow = gh.row;
            gh.cloneCol = gh.col;
            gh.cloneDir = oppositeDir(gh.dir);
            gh.cloneTimer = 420 + std::rand() % 250;
        }
    } else {
        if (nearCenter(gh.cloneX, gh.cloneY, gh.cloneRow, gh.cloneCol)) {
            gh.cloneX = cellToPixX(gh.cloneCol);
            gh.cloneY = cellToPixY(gh.cloneRow);
            gh.cloneDir = randomDir(gh.cloneRow, gh.cloneCol, gh.cloneDir);
        }

        if (canMoveDir(gh.cloneRow, gh.cloneCol, gh.cloneDir)) {
            int tr = gh.cloneRow + DR[gh.cloneDir];
            int tc = wrapCol(gh.cloneCol + DC[gh.cloneDir]);
            float tx = cellToPixX(tc);
            float ty = cellToPixY(tr);
            float sp = 1.5f;
            float vx = (tx > gh.cloneX) ? sp : ((tx < gh.cloneX) ? -sp : 0.0f);
            float vy = (ty > gh.cloneY) ? sp : ((ty < gh.cloneY) ? -sp : 0.0f);
            if (std::fabs(tx - gh.cloneX) <= std::fabs(vx) && std::fabs(ty - gh.cloneY) <= std::fabs(vy)) {
                gh.cloneX = tx;
                gh.cloneY = ty;
                gh.cloneRow = tr;
                gh.cloneCol = tc;
            } else {
                gh.cloneX += vx;
                gh.cloneY += vy;
            }
        }

        float dx = pac.x - gh.cloneX;
        float dy = pac.y - gh.cloneY;
        if (std::sqrt(dx * dx + dy * dy) < CELL * 0.65f && !pac.powerMode) {
            gh.cloneActive = false;
            loseLife();
        }
    }
}

void updateGhost(Ghost& gh) {
    if (!gh.alive) {
        updateBlinkyFire(gh);
        return;
    }

    if (gh.scared) {
        gh.scaredTimer--;
        if (gh.scaredTimer <= 0) gh.scared = false;
    }

    moveGhostBody(gh);
    updateBlinkyFire(gh);
    updateClydeClone(gh);
}

void checkCollisions() {
    const float COLLIDE_R = CELL * 0.7f;

    for (int i = 0; i < 4; i++) {
        Ghost& gh = ghosts[i];
        if (!gh.alive) continue;

        float dx = pac.x - gh.x;
        float dy = pac.y - gh.y;
        if (std::sqrt(dx * dx + dy * dy) < COLLIDE_R) {
            if (pac.powerMode && gh.scared) {
                gh.alive = false;
                gh.scared = false;
                score += 200;
            } else if (!pac.powerMode) {
                loseLife();
                return;
            }
        }
    }
}

void updateGame() {
    if (score > highScore) highScore = score;
    if ((elapsedMs / 5000) > 0 && elapsedMs % 5000 < 20) {
        for (int i = 0; i < 4; i++) {
            ghosts[i].speed = std::min(4.0f, ghosts[i].speed + 0.03f);
        }
    }
}

void timer(int value) {
    if (gameState == PLAYING) {
        elapsedMs += 16;
        updatePacman();
        for (int i = 0; i < 4; i++) updateGhost(ghosts[i]);
        checkCollisions();
        updateGame();
    } else if (gameState == MENU || gameState == GAME_OVER || gameState == WIN) {
        elapsedMs += 16;
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int x, int y) {
    if (key >= 'A' && key <= 'Z') key = key - 'A' + 'a';

    if (gameState == MENU) {
        if (key == 13 || key == 's') {
            if (menuChoice == 0) resetGame(true);
            else if (menuChoice == 1) { helpFromMenu = true; previousState = MENU; gameState = HELP; }
            else if (menuChoice == 2) std::exit(0);
        } else if (key == 'h') {
            helpFromMenu = true;
            previousState = MENU;
            gameState = HELP;
        } else if (key == 27 || key == 'q') {
            std::exit(0);
        }
        return;
    }

    if (gameState == PLAYING) {
        if (key == 'p') gameState = PAUSED;
        else if (key == 'h') { helpFromMenu = false; previousState = PLAYING; gameState = HELP; }
        else if (key == 27) gameState = MENU;
        else if (key == 'q') std::exit(0);
        return;
    }

    if (gameState == PAUSED) {
        if (key == 'p') gameState = PLAYING;
        else if (key == 27) gameState = MENU;
        return;
    }

    if (gameState == HELP) {
        if (key == 27) gameState = helpFromMenu ? MENU : previousState;
        return;
    }

    if (gameState == GAME_OVER || gameState == WIN) {
        if (key == 'm' || key == 27) gameState = MENU;
        else if (key == 'r') resetGame(true);
        else if (key == 'q') std::exit(0);
    }
}

void specialKeys(int key, int x, int y) {
    if (gameState == MENU) {
        if (key == GLUT_KEY_UP) {
            menuChoice--;
            if (menuChoice < 0) menuChoice = 2;
        } else if (key == GLUT_KEY_DOWN) {
            menuChoice++;
            if (menuChoice > 2) menuChoice = 0;
        }
        return;
    }

    if (gameState != PLAYING) return;

    if (key == GLUT_KEY_RIGHT) pac.nextDir = RIGHT;
    else if (key == GLUT_KEY_LEFT) pac.nextDir = LEFT;
    else if (key == GLUT_KEY_UP) pac.nextDir = UP;
    else if (key == GLUT_KEY_DOWN) pac.nextDir = DOWN;
}

void myinit() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WIN_W, 0.0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(1.0f);
    std::memcpy(originalMaze, maze, sizeof(maze));

    pac.row = 20;
    pac.col = 10;
    pac.x = cellToPixX(pac.col);
    pac.y = cellToPixY(pac.row);
    pac.dir = LEFT;
    pac.nextDir = LEFT;
    pac.speed = 2.0f;
    pac.mouthAngle = 0.35f;
    pac.mouthOpening = true;
    pac.powerMode = false;
    pac.powerTimer = 0;

    for (int i = 0; i < 4; i++) spawnGhost(i);
}

int main(int argc, char** argv) {
    std::srand((unsigned int)std::time(0));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(80, 40);
    glutCreateWindow("PAC-MAN | CSE426 Computer Graphics Lab");
    myinit();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
