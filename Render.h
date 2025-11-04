#include "imgui.h"
#include "clipper2/clipper.h"
#include <vector>
#include <cmath>

inline std::vector<ImVec2> Paths64ToImVec2(Clipper2Lib::Paths64&p){
    std::vector<ImVec2>c;
    for (auto& i : p) {
        for (auto& j : i) {
            c.push_back(ImVec2(j.x / 5882.f, j.y / 5882.f));
        }
    }
    return c;
}
class Vector2DPlot {
public:
    Vector2DPlot()
        : zoom_(1.0f), pan_offset_(0.0f, 0.0f), dragging_(false) {
    }

    void Begin(const char* title, const ImVec2& size = ImVec2(400, 400)) {
        ImGui::Begin(title);

        draw_list_ = ImGui::GetWindowDrawList();
        canvas_size_ = size;
        canvas_min_ = ImGui::GetCursorScreenPos();
        canvas_max_ = ImVec2(canvas_min_.x + size.x, canvas_min_.y + size.y);

        ImGui::InvisibleButton("canvas", size,
            ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
            ImGuiButtonFlags_MouseButtonMiddle);

        draw_list_->AddRectFilled(canvas_min_, canvas_max_, IM_COL32(255, 255, 255, 255));
        draw_list_->AddRect(canvas_min_, canvas_max_, IM_COL32(255, 255, 255, 255));

        origin_ = ImVec2((canvas_min_.x + canvas_max_.x) * 0.5f,
            (canvas_min_.y + canvas_max_.y) * 0.5f);

        HandleMouseInteraction();
        DrawAxes();
    }

    void DrawLine(const ImVec2& a, const ImVec2& b, ImU32 color, float thickness = 1.0f) {
        draw_list_->AddLine(ToScreen(a), ToScreen(b), color, thickness);
    }

    void DrawPolygon(const std::vector<ImVec2>& points, ImU32 color, bool filled = false, float thickness = 1.0f) {
        std::vector<ImVec2> pts;
        pts.reserve(points.size());
        for (auto& p : points) pts.push_back(ToScreen(p));
        if (filled)
            draw_list_->AddConvexPolyFilled(pts.data(), (int)pts.size(), color);
        else
            draw_list_->AddPolyline(pts.data(), (int)pts.size(), color, false, thickness);
    }

    void DrawCircle(const ImVec2& center, float radius, ImU32 color, bool filled = false, float thickness = 1.0f) {
        ImVec2 c = ToScreen(center);
        float r = radius * zoom_;
        if (filled)
            draw_list_->AddCircleFilled(c, r, color);
        else
            draw_list_->AddCircle(c, r, color, 0, thickness);
    }

    void DrawBezier(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 color, float thickness = 1.0f) {
        draw_list_->AddBezierCubic(ToScreen(p1), ToScreen(p2), ToScreen(p3), ToScreen(p4), color, thickness);
    }

    void DrawPaths64(const Clipper2Lib::Paths64& p,uint32_t color) {
        std::vector<std::vector<ImVec2>>counters;
        for (auto& i : p) {
            std::vector<ImVec2>c;
            for (auto& j : i) {
                c.push_back(ImVec2(j.x / 5882.f, j.y / 5882.f));

            }
            counters.push_back(c);
        }
        counterss.push_back(counters);

        colors.push_back(color);
    }

    void Render() {
        for (int i = 0; i < counterss.size(); i++) {
            for (int j = 0; j < counterss[i].size(); j++) {
                DrawPolygon(counterss[i][j],colors[i], false, 2.0f);
            }
        }
    }

    void ClearScreen() {
        counterss.clear();
        colors.clear();
    }
    void End() {
        ImGui::End();
    }

private:
    std::vector<std::vector<std::vector< ImVec2>>> counterss;
    std::vector<uint32_t>colors;
    

    // === 坐标变换 ===
    ImVec2 ToScreen(const ImVec2& p) const {
        // 世界坐标 -> 屏幕坐标
        float x = origin_.x + (p.x * zoom_) + pan_offset_.x;
        float y = origin_.y - (p.y * zoom_) + pan_offset_.y;
        return ImVec2(x, y);
    }

    // === 绘制坐标轴 ===
    void DrawAxes() {
        ImVec2 x1 = ToScreen(ImVec2(-1000.0f, 0.0f));
        ImVec2 x2 = ToScreen(ImVec2(1000.0f, 0.0f));
        ImVec2 y1 = ToScreen(ImVec2(0.0f, -1000.0f));
        ImVec2 y2 = ToScreen(ImVec2(0.0f, 1000.0f));
        draw_list_->AddLine(x1, x2, IM_COL32(200, 200, 200, 255), 1.0f);
        draw_list_->AddLine(y1, y2, IM_COL32(200, 200, 200, 255), 1.0f);
    }

    // === 鼠标交互 ===
    void HandleMouseInteraction() {
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mouse_pos = io.MousePos;

        bool hovered = ImGui::IsItemHovered();
        bool right_drag = ImGui::IsMouseDragging(ImGuiMouseButton_Right);

        // 鼠标右键拖动平移
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            dragging_ = true;
            drag_start_ = mouse_pos;
            last_pan_offset_ = pan_offset_;
        }

        if (dragging_ && right_drag) {
            ImVec2 delta = ImVec2(mouse_pos.x - drag_start_.x, mouse_pos.y - drag_start_.y);
            pan_offset_ = ImVec2(last_pan_offset_.x + delta.x, last_pan_offset_.y + delta.y);
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            dragging_ = false;

        // 鼠标滚轮缩放
        if (hovered && io.MouseWheel != 0.0f) {
            float zoom_before = zoom_;
            zoom_ *= (io.MouseWheel > 0 ? 1.1f : 0.9f);
            zoom_ = std::clamp(zoom_, 0.1f, 1000.0f);

            // 缩放中心在鼠标位置
            ImVec2 mouse_rel = ImVec2(mouse_pos.x - origin_.x - pan_offset_.x,
                mouse_pos.y - origin_.y - pan_offset_.y);
            pan_offset_.x -= mouse_rel.x * (zoom_ / zoom_before - 1.0f);
            pan_offset_.y -= mouse_rel.y * (zoom_ / zoom_before - 1.0f);
        }

        // 双击重置视图
        if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            zoom_ = 1.0f;
            pan_offset_ = ImVec2(0, 0);
        }
    }

private:
    ImDrawList* draw_list_ = nullptr;
    ImVec2 origin_;
    ImVec2 canvas_min_, canvas_max_, canvas_size_;

    float zoom_;
    ImVec2 pan_offset_;
    bool dragging_;
    ImVec2 drag_start_;
    ImVec2 last_pan_offset_;
};
