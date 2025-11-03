#include "imgui.h"
#include <vector>
#include <array>
#include <cstdlib>
#include <ctime>

constexpr int GridWidth = 10;
constexpr int GridHeight = 20;

std::array<std::array<int, GridWidth>, GridHeight> grid;

// 初始化网格
void InitGrid() {
    for (auto& row : grid) {
        row.fill(0);
    }
}

// 渲染网格
void RenderGrid() {
    for (int y = 0; y < GridHeight; ++y) {
        for (int x = 0; x < GridWidth; ++x) {
            ImGui::PushID(y * GridWidth + x);
            ImVec4 color = grid[y][x] ? ImVec4(0.0f, 0.6f, 1.0f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, color);
            ImGui::SetCursorPos(ImVec2(x * 20.0f, y * 20.0f));
            ImGui::Button("", ImVec2(20, 20));
            ImGui::PopStyleColor();
            ImGui::PopID();
           // if (x < GridWidth - 1) ImGui::SameLine();
        }
    }
}



struct Tetromino {
    std::vector<std::vector<int>> shape;
    ImVec4 color;
};

// 一些标准的俄罗斯方块形状
std::vector<Tetromino> tetrominoes = {
    { { {1, 1, 1, 1} }, ImVec4(0.0f, 0.0f, 1.0f, 1.0f) },  // I
    { { {1, 1}, {1, 1} }, ImVec4(1.0f, 0.85f, 0.0f, 1.0f) },  // O
    { { {0, 1, 0}, {1, 1, 1} }, ImVec4(1.0f, 0.0f, 0.0f, 1.0f) },  // T
    { { {1, 1, 0}, {0, 1, 1} }, ImVec4(0.0f, 1.0f, 0.0f, 1.0f) },  // S
    { { {0, 1, 1}, {1, 1, 0} }, ImVec4(1.0f, 0.0f, 1.0f, 1.0f) },  // Z
    { { {1, 1, 1}, {1, 0, 0} }, ImVec4(1.0f, 0.5f, 0.0f, 1.0f) },  // L
    { { {1, 1, 1}, {0, 0, 1} }, ImVec4(0.0f, 1.0f, 1.0f, 1.0f) },  // J
};

Tetromino currentTetromino;
int currentX = GridWidth / 2 - 1;
int currentY = 0;

// 生成新的方块
void SpawnTetromino() {
    currentTetromino = tetrominoes[rand() % tetrominoes.size()];
    currentX = GridWidth / 2 - 1;
    currentY = 0;
}


bool CheckCollision(int newX, int newY, const std::vector<std::vector<int>>& shape) {
    for (int y = 0; y < shape.size(); ++y) {
        for (int x = 0; x < shape[y].size(); ++x) {
            if (shape[y][x]) {
                int gridX = newX + x;
                int gridY = newY + y;
                if (gridX < 0 || gridX >= GridWidth || gridY < 0 || gridY >= GridHeight || grid[gridY][gridX])
                    return true;
            }
        }
    }
    return false;
}

void MoveTetromino(int dx, int dy) {
    if (!CheckCollision(currentX + dx, currentY + dy, currentTetromino.shape)) {
        currentX += dx;
        currentY += dy;
    }
}

// 放置方块到网格
void PlaceTetromino() {
    for (int y = 0; y < currentTetromino.shape.size(); ++y) {
        for (int x = 0; x < currentTetromino.shape[y].size(); ++x) {
            if (currentTetromino.shape[y][x]) {
                grid[currentY + y][currentX + x] = 1;
            }
        }
    }
    // 生成新方块
    SpawnTetromino();
}

// 检查并清除满行
void ClearLines() {
    for (int y = 0; y < GridHeight; ++y) {
        bool full = true;
        for (int x = 0; x < GridWidth; ++x) {
            if (!grid[y][x]) {
                full = false;
                break;
            }
        }
        if (full) {
            for (int moveY = y; moveY > 0; --moveY) {
                grid[moveY] = grid[moveY - 1];
            }
            grid[0].fill(0);
        }
    }
}


std::vector<std::vector<int>> Rotate90Degrees(const std::vector<std::vector<int>>& shape) {
    int n = shape.size();
    int m = shape[0].size();

    // 创建一个 m x n 的新矩阵来保存旋转后的形状
    std::vector<std::vector<int>> rotatedShape(m, std::vector<int>(n, 0));

    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < m; ++x) {
            rotatedShape[x][n - 1 - y] = shape[y][x];
        }
    }

    return rotatedShape;
}

void UpdateGame() {
    // 用户输入
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow))) {
        MoveTetromino(-1, 0);
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow))) {
        MoveTetromino(1, 0);
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) {
        MoveTetromino(0, 1);
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) {
        auto rotatedShape = Rotate90Degrees(currentTetromino.shape);

        // 检查旋转后的形状是否会与其他方块或边界发生碰撞
        if (!CheckCollision(currentX, currentY, rotatedShape)) {
            currentTetromino.shape = rotatedShape;
        }
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace))) {
        InitGrid();
    }

    // 模拟自动下落
    static float lastTime = 0.0f;
    float currentTime = ImGui::GetTime();
    if (currentTime - lastTime > 0.5f) {  // 控制下落速度
        if (!CheckCollision(currentX, currentY + 1, currentTetromino.shape)) {
            MoveTetromino(0, 1);
        }
        else {
            PlaceTetromino();
            ClearLines();
        }
        lastTime = currentTime;
    }

    RenderGrid();

    // 渲染当前方块
    for (int y = 0; y < currentTetromino.shape.size(); ++y) {
        for (int x = 0; x < currentTetromino.shape[y].size(); ++x) {
            if (currentTetromino.shape[y][x]) {
                int drawX = currentX + x;
                int drawY = currentY + y;
                ImGui::SetCursorPosX(drawX * 20.0f);
                ImGui::SetCursorPosY(drawY * 20.0f);
                ImGui::Button(" ", ImVec2(20, 20));
            }
        }
    }
    ImGui::SetCursorPos(ImVec2(0, 0));
    // 渲染网格
    
}
