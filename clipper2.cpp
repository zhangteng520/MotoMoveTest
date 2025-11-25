#include <algorithm> 
#include "clipper2.h"
#include "imgui.h"

using namespace std;
using namespace Clipper2Lib;

void Rotate1()
{
    Paths64 op1, op2;
    FillRule fr2 = FillRule::Positive;


    SvgWriter svg2;
    PathD a = { {30, 150}, {60, 350}, {0, 350} };
    PathD b = { {30, 150}, {60, 350}, {0, 350} };
    for (int i = 0; i < 3; i++) {
        b[i].x = cos(1) * a[i].x + sin(1) * a[i].y;
        b[i].y = cos(1) * a[i].y - sin(1) * a[i].x;
    }
    PathsD p;
    p.push_back(b);
    p.push_back(a);
    cout << b[1].x << endl;
    SvgAddSolution(svg2, p, FillRule::Negative, true);
    //SvgAddOpenSubject(svg2, p, fr2, false);
    SvgSaveToFile(svg2, "open_paths.svg", 800, 600, 0);
    System("open_paths.svg");
}

void System(const std::string& filename)
{
#ifdef _WIN32
    system(filename.c_str());
#else
    system(("firefox " + filename).c_str());
#endif
}
Clipper2Lib::Paths64 CBoundaryFloatToInt64(const CBoundary& Cboundary) {
    Clipper2Lib::Paths64 temp;

    temp.clear();
    for (auto i : Cboundary) {
        Clipper2Lib::Path64 temp1;
        for (auto j : i)
        {
            Clipper2Lib::Point64 temp2;
            temp2.x = j.x * galvanometerRatio;
            temp2.y = j.y * galvanometerRatio;
            temp1.push_back(temp2);
        }
        temp.push_back(temp1);
    }
    return temp;
}
void BeamCompensation(Clipper2Lib::Paths64& paths64, int beamRadius) {
    JoinType jt = JoinType::Round;
    paths64 = InflatePaths(paths64, -beamRadius, jt, EndType::Polygon);
}


Edges Paths64ToEdges(Clipper2Lib::Paths64& paths64, bool IsScanLineSLevel) {
    Edges edges;
    if (IsScanLineSLevel) {
        for (auto& path64 : paths64) {
            for (int i = 0; i < path64.size(); i++) {
                int j = (i + 1) % path64.size();
                //处理水平线
                if (path64[i].y != path64[j].y) {
                    //整理出每条边的ymin , x_ymin ,ymax , m,生成edge
                    Edge edge;
                    edge.ymin = min(path64[i].y, path64[j].y);
                    edge.x_ymin = (edge.ymin == path64[i].y) ? (path64[i].x) : (path64[j].x);
                    edge.ymax = max(path64[i].y, path64[j].y);
                    edge.m = (static_cast<double>(path64[i].x - path64[j].x)) / (path64[i].y - path64[j].y);

                    //不处理极值点
                    /*if(edges.back().m * edge.m < 0&&edges.empty()!= true)
                        edges.push_back(edge);*/
                    edges.push_back(edge);
                }
            }
        }
    }
    else {
        for (auto& path64 : paths64) {
            for (int i = 0; i < path64.size(); i++) {
                int j = (i + 1) % path64.size();
                //处理垂直线
                if (path64[i].x != path64[j].x) {
                    //整理出每条边的ymin , x_ymin ,ymax , m,生成edge
                    Edge edge;
                    edge.ymin = min(path64[i].x, path64[j].x);
                    edge.x_ymin = (edge.ymin == path64[i].x) ? (path64[i].y) : (path64[j].y);
                    edge.ymax = max(path64[i].x, path64[j].x);
                    edge.m = (static_cast<double>(path64[i].y - path64[j].y)) / (path64[i].x - path64[j].x);


                    edges.push_back(edge);
                }
            }
        }
    }

    return edges;
}


void EdgesToScanPaths(Edges& edges, int64_t interval, bool IsLowToHigh, bool IsScanLineSLevel, Clipper2Lib::Paths64& paths64) {
    if (edges.size() == 0)
        return;
    
    //查询Ymax
    auto Ymax_iter = std::max_element(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
        return a.ymax < b.ymax; });

    int64_t Ymax = Ymax_iter->ymax;
    //按照ymin排序
    //std::sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
    //    return a.ymin < b.ymin; });
    //查询Ymin
    auto Ymin_iter = std::min_element(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
        return a.ymin < b.ymin; });
    int64_t Ymin = Ymin_iter->ymin;

    //开始扫描
    if (IsLowToHigh) {
        for (int64_t scanLineX = Ymin, count = 0; scanLineX < Ymax; scanLineX += interval, count++) {
            //收集活性边
            Edges AET;
            for (auto& edge : edges) {
                if (edge.ymin <= scanLineX && edge.ymax > scanLineX)
                    AET.push_back(edge);
            }
            if (AET.empty() == true)
                continue;

            //计算交点
            std::vector<int64_t> intersection;

            for (auto& aet : AET) {
                int64_t x = aet.x_ymin + aet.m * (scanLineX - aet.ymin);
                intersection.push_back(x);
            }


            //交点排序

            if (count % 2 == 0)
                std::sort(intersection.begin(), intersection.end(), [](const int64_t& a, const int64_t& b)
                    { return a < b; });
            else
                std::sort(intersection.begin(), intersection.end(), [](const int64_t& a, const int64_t& b)
                    { return a > b; });

            //制作扫描路径

            for (int64_t i = 0; i < intersection.size(); i += 2) {
                if (intersection[i] == intersection[i + 1])
                    continue;
                if (IsScanLineSLevel) {
                    Point64 point64_1(intersection[i], scanLineX);
                    Point64 point64_2(intersection[i + 1], scanLineX);
                    Path64 path64;
                    path64.push_back(point64_1);
                    path64.push_back(point64_2);
                    paths64.push_back(path64);
                }
                else {
                    Point64 point64_1(scanLineX, intersection[i]);
                    Point64 point64_2(scanLineX, intersection[i + 1]);
                    Path64 path64;
                    path64.push_back(point64_1);
                    path64.push_back(point64_2);
                    paths64.push_back(path64);
                }


            }


        }

    }
    else {
        for (int64_t scanLineX = Ymax, count = 0; scanLineX > Ymin; scanLineX -= interval, count++) {
            //收集活性边
            Edges AET;
            for (auto& edge : edges) {
                if (edge.ymin <= scanLineX && edge.ymax > scanLineX)
                    AET.push_back(edge);
            }
            if (AET.empty() == true)
                continue;

            //计算交点
            std::vector<int64_t> intersection;

            for (auto& aet : AET) {
                int64_t x = aet.x_ymin + aet.m * (scanLineX - aet.ymin);
                intersection.push_back(x);
            }


            //交点排序

            if (count % 2 == 0)
                std::sort(intersection.begin(), intersection.end(), [](const int64_t& a, const int64_t& b)
                    { return a < b; });
            else
                std::sort(intersection.begin(), intersection.end(), [](const int64_t& a, const int64_t& b)
                    { return a > b; });

            //制作扫描路径

            for (int64_t i = 0; i < intersection.size(); i += 2) {
                if (intersection[i] == intersection[i + 1])
                    continue;
                if (IsScanLineSLevel) {
                    Point64 point64_1(intersection[i], scanLineX);
                    Point64 point64_2(intersection[i + 1], scanLineX);
                    Path64 path64;
                    path64.push_back(point64_1);
                    path64.push_back(point64_2);
                    paths64.push_back(path64);
                }
                else {
                    Point64 point64_1(scanLineX, intersection[i]);
                    Point64 point64_2(scanLineX, intersection[i + 1]);
                    Path64 path64;
                    path64.push_back(point64_1);
                    path64.push_back(point64_2);
                    paths64.push_back(path64);
                }


            }

        }
    }
}
void EdgesToRaster(Edges& edges, int64_t interval, bool IsLowToHigh, bool IsScanLineSLevel, Clipper2Lib::Paths64& paths64) {
    if (edges.size() == 0)
        return;
    //查询Ymax
    auto Ymax_iter = std::max_element(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
        return a.ymax < b.ymax; });

    int64_t Ymax = Ymax_iter->ymax;
    //查询Ymin
    auto Ymin_iter = std::min_element(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
        return a.ymin < b.ymin; });
    int64_t Ymin = Ymin_iter->ymin;

    //开始扫描
    if (IsLowToHigh) {
        for (int64_t scanLineX = Ymin, count = 0; scanLineX < Ymax; scanLineX += interval, count++) {
            //收集活性边
            Edges AET;
            for (auto& edge : edges) {   
                if (edge.ymin <= scanLineX && edge.ymax > scanLineX) {
                    AET.push_back(edge);
                }
            }
            if (AET.empty() == true)
                continue;

            assert(AET.size() % 2 == 0);

            //计算交点
            std::vector<int64_t> intersection;

            for (auto& aet : AET) {
                int64_t x = aet.x_ymin + aet.m * (scanLineX - aet.ymin);
                intersection.push_back(x);
            }


            //交点排序
            std::sort(intersection.begin(), intersection.end(), [](const int64_t& a, const int64_t& b)
                { return a < b; });

            //制作扫描路径

            for (int64_t i = 0; i < intersection.size(); i += 2) {
                if (intersection[i] == intersection[i + 1])
                    continue;
                if (IsScanLineSLevel) {
                    Point64 point64_1(intersection[i], scanLineX);
                    Point64 point64_2(intersection[i + 1], scanLineX);
                    Path64 path64;
                    path64.push_back(point64_1);
                    path64.push_back(point64_2);
                    paths64.push_back(path64);
                }
                else {
                    Point64 point64_1(scanLineX, intersection[i]);
                    Point64 point64_2(scanLineX, intersection[i + 1]);
                    Path64 path64;
                    path64.push_back(point64_1);
                    path64.push_back(point64_2);
                    paths64.push_back(path64);
                }


            }


        }
    }

    else {
        for (int64_t scanLineX = Ymax - interval, count = 0; scanLineX > Ymin; scanLineX -= interval, count++) {
            //收集活性边
            Edges AET;
            for (auto& edge : edges) {
                if (edge.ymin <= scanLineX && edge.ymax > scanLineX)
                    AET.push_back(edge);
            }
            if (AET.empty() == true)
                continue;

            //计算交点
            std::vector<int64_t> intersection;

            for (auto& aet : AET) {
                int64_t x = aet.x_ymin + aet.m * (scanLineX - aet.ymin);
                intersection.push_back(x);
            }


            //交点排序
            std::sort(intersection.begin(), intersection.end(), [](const int64_t& a, const int64_t& b)
                { return a < b; });

            //制作扫描路径

            for (int64_t i = 0; i < intersection.size(); i += 2) {
                if (intersection[i] == intersection[i + 1])
                    continue;
                if (IsScanLineSLevel) {
                    Point64 point64_1(intersection[i], scanLineX);
                    Point64 point64_2(intersection[i + 1], scanLineX);
                    Path64 path64;
                    path64.push_back(point64_1);
                    path64.push_back(point64_2);
                    paths64.push_back(path64);
                }
                else {
                    Point64 point64_1(scanLineX, intersection[i]);
                    Point64 point64_2(scanLineX, intersection[i + 1]);
                    Path64 path64;
                    path64.push_back(point64_1);
                    path64.push_back(point64_2);
                    paths64.push_back(path64);
                }

            }

        }
    }

}
//返回scanpathss64的线段总条数
int EdgesToScanPathsFast(Edges& edges, int64_t interval, bool IsLowToHigh, bool IsScanLinesLevel, ScanPathss64& scanpathss64) {
    int TotalNumberOfSegments = 0;
    if (edges.size() == 0)
        return 0;
    //查询Ymax
    auto Ymax_iter = std::max_element(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
        return a.ymax < b.ymax; });

    int64_t Ymax = Ymax_iter->ymax;
    //按照ymin排序
    //std::sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
    //    return a.ymin < b.ymin; });
    //查询Ymin
    auto Ymin_iter = std::min_element(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
        return a.ymin < b.ymin; });
    int64_t Ymin = Ymin_iter->ymin;

    //开始扫描
   // Point64 PointLast;
    if (IsLowToHigh) {
        for (int64_t scanLineX = Ymin + interval; scanLineX <= Ymax; scanLineX += interval) {
            //收集活性边
            Edges AET;
            for (auto& edge : edges) {
                if (edge.ymin <= scanLineX && edge.ymax > scanLineX)
                    AET.push_back(edge);
            }
            if (AET.empty() == true)
                continue;

            //计算交点
            std::vector<int64_t> intersection;

            for (auto& aet : AET) {
                int64_t x = aet.x_ymin + aet.m * (scanLineX - aet.ymin);
                intersection.push_back(x);
            }


            //交点排序

            std::sort(intersection.begin(), intersection.end(), [](const int64_t& a, const int64_t& b)
                { return a < b; });
            //制作有层信息的路径
            ScanPaths64 scanpaths64;
            for (int i = 0; i < intersection.size(); i += 2) {
                if (intersection[i] == intersection[i + 1])
                    continue;
                if (IsScanLinesLevel) {
                    Point64 point64_1(intersection[i], scanLineX);
                    Point64 point64_2(intersection[i + 1], scanLineX);
                    ScanPath64 scanpath64;
                    scanpath64.path64.push_back(point64_1);
                    scanpath64.path64.push_back(point64_2);
                    scanpaths64.scanpaths64.push_back(scanpath64);
                    TotalNumberOfSegments++;
                }
                else {
                    Point64 point64_1(scanLineX, intersection[i]);
                    Point64 point64_2(scanLineX, intersection[i + 1]);
                    ScanPath64 scanpath64;
                    scanpath64.path64.push_back(point64_1);
                    scanpath64.path64.push_back(point64_2);
                    scanpaths64.scanpaths64.push_back(scanpath64);
                    TotalNumberOfSegments++;
                }

            }
            if (scanpaths64.scanpaths64.size() != 0) {
                scanpathss64.push_back(scanpaths64);
            }

        }
    }
    else {
        for (int64_t scanLineX = Ymax - interval; scanLineX >= Ymin; scanLineX -= interval) {
            //收集活性边
            Edges AET;
            for (auto& edge : edges) {
                if (edge.ymin <= scanLineX && edge.ymax > scanLineX)
                    AET.push_back(edge);
            }
            if (AET.empty() == true)
                continue;

            //计算交点
            std::vector<int64_t> intersection;

            for (auto& aet : AET) {
                int64_t x = aet.x_ymin + aet.m * (scanLineX - aet.ymin);
                intersection.push_back(x);
            }


            //交点排序

            std::sort(intersection.begin(), intersection.end(), [](const int64_t& a, const int64_t& b)
                { return a < b; });
            //制作有层信息的路径
            ScanPaths64 scanpaths64;
            for (int i = 0; i < intersection.size(); i += 2) {
                if (intersection[i] == intersection[i + 1])
                    continue;
                Point64 point64_1(intersection[i], scanLineX);
                Point64 point64_2(intersection[i + 1], scanLineX);
                ScanPath64 scanpath64;
                scanpath64.path64.push_back(point64_1);
                scanpath64.path64.push_back(point64_2);
                scanpaths64.scanpaths64.push_back(scanpath64);
                TotalNumberOfSegments++;
            }
            if (scanpaths64.scanpaths64.size() != 0) {
                scanpathss64.push_back(scanpaths64);
            }

        }
    }
    return TotalNumberOfSegments;
}

void RotateAngle(CBoundary& Cboundary, double angle) {
    angle = angle / PI_180;
    for (auto& i : Cboundary) {
        for (auto& j : i) {
            float temp_x = j.x;
            float temp_y = j.y;
            j.x = cos(angle) * temp_x + sin(angle) * temp_y;
            j.y = cos(angle) * temp_y - sin(angle) * temp_x;
        }
    }
}
void RotateAngle(Clipper2Lib::Paths64& paths64, double angle) {
    angle = angle / PI_180;//顺时针旋转
    for (auto& i : paths64) {
        for (auto& j : i) {
            int64_t temp_x = j.x;
            int64_t temp_y = j.y;
            j.x = cos(angle) * temp_x + sin(angle) * temp_y;
            j.y = cos(angle) * temp_y - sin(angle) * temp_x;
        }
    }
}
void RotateAngle(std::vector<Clipper2Lib::Paths64>& paths64, double angle) {
    angle = angle / PI_180;
    for (auto& z : paths64) {
        for (auto& i : z) {
            for (auto& j : i) {
                int64_t temp_x = j.x;
                int64_t temp_y = j.y;
                j.x = cos(angle) * temp_x + sin(angle) * temp_y;
                j.y = cos(angle) * temp_y - sin(angle) * temp_x;
            }
        }
    }
}
void RotateAngle(ScanPathss64& scanpathss64, double angle) {
    angle = angle / PI_180;//顺时针旋转
    for (auto& scanpaths64 : scanpathss64) {
        for (auto& scanpath64 : scanpaths64.scanpaths64) {
            for (auto& j : scanpath64.path64) {
                int64_t temp_x = j.x;
                int64_t temp_y = j.y;
                j.x = cos(angle) * temp_x + sin(angle) * temp_y;
                j.y = cos(angle) * temp_y - sin(angle) * temp_x;
            }
        }
    }
}

int ReduceHeliJump(ScanPathss64& scanpathss64, Clipper2Lib::Paths64& paths64, unsigned int probeTimes) {
    //统计有多少个线段
    unsigned int TotalNumber = 0;
    for (auto& scanpaths64 : scanpathss64) {
        TotalNumber += scanpaths64.scanpaths64.size();
    }
    //进入循环
    int count = 0;
    ScanPath64& traveler = scanpathss64[0].scanpaths64[0];
    int pos_x = 0;
    int pos_y = 0;
    traveler.IsScanPath64 = true;
    paths64.push_back(traveler.path64);
    while (count < TotalNumber) {
        //先查找左右，在查找上下，最后查找所有
        bool isFind = false;

    }
    return 0;
}
int ReduceHeliJump(Clipper2Lib::Paths64& paths64, Clipper2Lib::Paths64& temp) {
    Clipper2Lib::Path64& traveler = paths64[0];
    int pos = 0;
    int pos_sub = 0;//只能是0 or 1
    temp.push_back(traveler);
    //制作No号
    vector<int>No;
    for (int i = 1; i < paths64.size(); i++) {
        No.push_back(i);
    }
    for (int i = 1; i < paths64.size(); i++) {
        //找到最短距离
        int64_t DistanceMin = INT64_MAX;
        int pos_tmp = 0;
        for (auto j : No) {
            int64_t Distance0 = (paths64[j][0].x - paths64[pos][1].x) * (paths64[j][0].x - paths64[pos][1].x) +
                (paths64[j][0].y - paths64[pos][1].y) * (paths64[j][0].y - paths64[pos][1].y);
            int64_t Distance1 = (paths64[j][1].x - paths64[pos][1].x) * (paths64[j][1].x - paths64[pos][1].x) +
                (paths64[j][1].y - paths64[pos][1].y) * (paths64[j][1].y - paths64[pos][1].y);
            if (min(Distance0, Distance1) < DistanceMin) {
                if (Distance0 == min(Distance0, Distance1)) {
                    pos_sub = 0;
                    DistanceMin = Distance0;
                    pos_tmp = j;
                }
                else {
                    pos_sub = 1;
                    DistanceMin = Distance1;
                    pos_tmp = j;
                }
            }
        }
        pos = pos_tmp;
        if (pos_sub == 1)
            swap(paths64[pos][0], paths64[pos][1]);
        temp.push_back(paths64[pos]);
        std::remove(No.begin(), No.end(), pos);
        No.resize(No.size() - 1);
    }
    return 0;
}

int ReduceHeliJump(ScanPathss64& scanpathss64, const float tolerance_x, const int tolerance_y, Clipper2Lib::Paths64& output) {
    //根据线段条数计算循环次数
    int sizeOfSegments = 0;
    for (int i = 0; i < scanpathss64.size(); i++) {
        sizeOfSegments += scanpathss64[i].scanpaths64.size();
    }

    //进入循环
    int pos_x = 0;
    int pos_y = 0;
    int tolerance_count = 0;
    for (int z = 0; z < sizeOfSegments; z++) {
        //扫描过了此条线段
        scanpathss64[pos_y].scanpaths64[pos_x].IsScanPath64 = true;


        //是否水平线段全部扫描完成
        int count = 0;
        for (int i = 0; i < scanpathss64[pos_y].scanpaths64.size(); i++) {
            if (scanpathss64[pos_y].scanpaths64[i].IsScanPath64 == true)
                count++;
        }
        if (count == scanpathss64[pos_y].scanpaths64.size())
            scanpathss64[pos_y].IsScanPaths64 = true;

        output.push_back(scanpathss64[pos_y].scanpaths64[pos_x].path64);

        //寻找下一条线段
        if (z == 101)
            int ii = 0;
        // 向上查找
        if (tolerance_count > tolerance_y) {
            tolerance_count = 0;
            int tmp_x = 0;
            int tmp_y = 0;
            bool breakflag = false;
            for (int i = 0; i < scanpathss64.size() && breakflag == false; i++) {
                if (scanpathss64[i].IsScanPaths64 == false) {
                    for (int j = 0; j < scanpathss64[i].scanpaths64.size() && breakflag == false; j++) {
                        if (scanpathss64[i].scanpaths64[j].IsScanPath64 == false) {
                            tmp_x = j;
                            tmp_y = i;
                            breakflag = true;
                        }
                    }
                }
            }
            pos_x = tmp_x;
            pos_y = tmp_y;
        }
        //再找向下
        if (pos_y + 1 < scanpathss64.size() && scanpathss64[pos_y + 1].IsScanPaths64 == false) {
            int64_t DistanceMin = INT64_MAX;
            int pos_x_tmp = 0;
            int pos_sub = 1;
            for (int i = 0; i < scanpathss64[pos_y + 1].scanpaths64.size(); i++) {
                if (scanpathss64[pos_y + 1].scanpaths64[i].IsScanPath64 == false) {
                    int64_t distance0 = (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x - scanpathss64[pos_y + 1].scanpaths64[i].path64[0].x) * (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x - scanpathss64[pos_y + 1].scanpaths64[i].path64[0].x)
                        + (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].y - scanpathss64[pos_y + 1].scanpaths64[i].path64[0].y) * (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].y - scanpathss64[pos_y + 1].scanpaths64[i].path64[0].y);
                    int64_t distance1 = (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x - scanpathss64[pos_y + 1].scanpaths64[i].path64[1].x) * (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x - scanpathss64[pos_y + 1].scanpaths64[i].path64[1].x)
                        + (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].y - scanpathss64[pos_y + 1].scanpaths64[i].path64[1].y) * (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].y - scanpathss64[pos_y + 1].scanpaths64[i].path64[1].y);
                    if (min(distance0, distance1) < DistanceMin) {
                        if (distance0 == min(distance0, distance1)) {
                            pos_sub = 0;
                            DistanceMin = distance0;
                            pos_x_tmp = i;
                        }
                        else {
                            pos_sub = 1;
                            DistanceMin = distance1;
                            pos_x_tmp = i;
                        }
                    }
                }
            }
            if (pos_sub == 1)
                swap(scanpathss64[pos_y + 1].scanpaths64[pos_x_tmp].path64[0], scanpathss64[pos_y + 1].scanpaths64[pos_x_tmp].path64[1]);
            pos_x = pos_x_tmp;
            pos_y++;
            tolerance_count++;
            continue;
        }
        //先找水平左右
        if (scanpathss64[pos_y].IsScanPaths64 == false) {
            //先向右边寻找
            if ((pos_x + 1) < scanpathss64[pos_y].scanpaths64.size()) {
                if (scanpathss64[pos_y].scanpaths64[pos_x + 1].IsScanPath64 == false) {//当作0在左，1在右
                    int64_t markdistance_x = abs(scanpathss64[pos_y].scanpaths64[pos_x].path64[0].x - scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x);
                    int64_t jumpdistance_x = abs(scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x - scanpathss64[pos_y].scanpaths64[pos_x + 1].path64[0].x);
                    if (jumpdistance_x < tolerance_x * markdistance_x) {
                        pos_x++;
                        continue;
                    }
                }
            }
            //向左找寻找
            if ((pos_x - 1) >= 0) {
                if (scanpathss64[pos_y].scanpaths64[pos_x - 1].IsScanPath64 == false) {
                    int64_t markdistance_x = abs(scanpathss64[pos_y].scanpaths64[pos_x].path64[0].x - scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x);
                    int64_t jumpdistance_x = abs(scanpathss64[pos_y].scanpaths64[pos_x].path64[0].x - scanpathss64[pos_y].scanpaths64[pos_x - 1].path64[1].x);
                    if (jumpdistance_x < tolerance_x * markdistance_x) {
                        swap(scanpathss64[pos_y].scanpaths64[pos_x - 1].path64[0], scanpathss64[pos_y].scanpaths64[pos_x - 1].path64[1]);
                        pos_x--;
                        continue;
                    }
                }

            }

        }


        //最后全局寻找
        int tmp_x = 0;
        int tmp_y = 0;
        bool breakflag = false;
        for (int i = 0; i < scanpathss64.size() && breakflag == false; i++) {
            if (scanpathss64[i].IsScanPaths64 == false) {
                for (int j = 0; j < scanpathss64[i].scanpaths64.size() && breakflag == false; j++) {
                    if (scanpathss64[i].scanpaths64[j].IsScanPath64 == false) {
                        tmp_x = j;
                        tmp_y = i;
                        breakflag = true;
                    }
                }
            }
        }
        pos_x = tmp_x;
        pos_y = tmp_y;
    }
    return 0;
}

int ReduceHeliJump(ScanPathss64& scanpathss64, const float tolerance_x, bool IsAbsolute, Clipper2Lib::Paths64& output) {
    if (scanpathss64.size() != 0) {
        int pos_x = 0;
        int pos_y = 0;
        bool IsFindAll = false;
        while (!IsFindAll) {
            //扫描过了此条线段
            scanpathss64[pos_y].scanpaths64[pos_x].IsScanPath64 = true;


            //是否水平线段全部扫描完成
            int count = 0;
            for (int i = 0; i < scanpathss64[pos_y].scanpaths64.size(); i++) {
                if (scanpathss64[pos_y].scanpaths64[i].IsScanPath64 == true)
                    count++;
            }
            if (count == scanpathss64[pos_y].scanpaths64.size())
                scanpathss64[pos_y].IsScanPaths64 = true;

            output.push_back(scanpathss64[pos_y].scanpaths64[pos_x].path64);


            //先找左右的区域
            if (scanpathss64[pos_y].IsScanPaths64 == false) {
                int64_t  JumpMin = INT64_MAX;
                int64_t Mark = abs(scanpathss64[pos_y].scanpaths64[pos_x].path64[0].x - scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x);
                int pos_x_tmp = 0;
                int x_sub = 0;
                for (int i = 0; i < scanpathss64[pos_y].scanpaths64.size(); i++) {
                    if (scanpathss64[pos_y].scanpaths64[i].IsScanPath64 == false) {
                        if (i < pos_x) {
                            int64_t Jump = abs(scanpathss64[pos_y].scanpaths64[i].path64[1].x - scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x);
                            if (Jump < JumpMin) {
                                JumpMin = Jump;
                                x_sub = 1;
                                pos_x_tmp = i;
                            }
                        }
                        else {
                            int64_t Jump = abs(scanpathss64[pos_y].scanpaths64[i].path64[0].x - scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x);
                            if (Jump < JumpMin) {
                                JumpMin = Jump;
                                x_sub = 0;
                                pos_x_tmp = i;
                            }
                        }
                    }

                }
                if (!IsAbsolute) {
                    if (JumpMin < tolerance_x * Mark) {
                        if (x_sub == 1) {
                            swap(scanpathss64[pos_y].scanpaths64[pos_x_tmp].path64[0], scanpathss64[pos_y].scanpaths64[pos_x_tmp].path64[1]);
                            pos_x = pos_x_tmp;
                            continue;
                        }
                        else {
                            pos_x = pos_x_tmp;
                            continue;
                        }
                    }
                }
                else {
                    if (JumpMin < tolerance_x * 5882.f) {
                        if (x_sub == 1) {
                            swap(scanpathss64[pos_y].scanpaths64[pos_x_tmp].path64[0], scanpathss64[pos_y].scanpaths64[pos_x_tmp].path64[1]);
                            pos_x = pos_x_tmp;
                            continue;
                        }
                        else {
                            pos_x = pos_x_tmp;
                            continue;
                        }
                    }
                }
            }

            //先向下找到有交集的线段
            if (pos_y + 1 < scanpathss64.size() && scanpathss64[pos_y + 1].IsScanPaths64 == false) {
                int64_t Mark1 = abs(scanpathss64[pos_y].scanpaths64[pos_x].path64[0].x - scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x);
                int64_t DistanceMin = INT64_MAX;
                int pos_x_tmp = 0;
                int pos_sub = 1;
                for (int i = 0; i < scanpathss64[pos_y + 1].scanpaths64.size(); i++) {
                    if (scanpathss64[pos_y + 1].scanpaths64[i].IsScanPath64 == false) {
                        int64_t distance0 = (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x - scanpathss64[pos_y + 1].scanpaths64[i].path64[0].x) * (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x - scanpathss64[pos_y + 1].scanpaths64[i].path64[0].x)
                            + (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].y - scanpathss64[pos_y + 1].scanpaths64[i].path64[0].y) * (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].y - scanpathss64[pos_y + 1].scanpaths64[i].path64[0].y);
                        int64_t distance1 = (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x - scanpathss64[pos_y + 1].scanpaths64[i].path64[1].x) * (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].x - scanpathss64[pos_y + 1].scanpaths64[i].path64[1].x)
                            + (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].y - scanpathss64[pos_y + 1].scanpaths64[i].path64[1].y) * (scanpathss64[pos_y].scanpaths64[pos_x].path64[1].y - scanpathss64[pos_y + 1].scanpaths64[i].path64[1].y);
                        if (min(distance0, distance1) < DistanceMin) {
                            if (distance0 == min(distance0, distance1)) {//少了一个等于号
                                pos_sub = 0;
                                DistanceMin = distance0;
                                pos_x_tmp = i;
                            }
                            else {
                                pos_sub = 1;
                                DistanceMin = distance1;
                                pos_x_tmp = i;
                            }
                        }
                    }
                }
                int64_t Mark2 = abs(scanpathss64[pos_y + 1].scanpaths64[pos_x_tmp].path64[0].x - scanpathss64[pos_y + 1].scanpaths64[pos_x_tmp].path64[1].x);
                if (Mark2 < Mark1 * 1.5f) {
                    if (pos_sub == 1)
                        swap(scanpathss64[pos_y + 1].scanpaths64[pos_x_tmp].path64[0], scanpathss64[pos_y + 1].scanpaths64[pos_x_tmp].path64[1]);
                    pos_x = pos_x_tmp;
                    pos_y++;
                    continue;
                }

            }
            //最后全局查找
            bool breakflag = false;
            for (int i = 0; i < scanpathss64.size() && breakflag == false; i++) {
                if (scanpathss64[i].IsScanPaths64 == false) {
                    for (int j = 0; j < scanpathss64[i].scanpaths64.size() && breakflag == false; j++) {
                        if (scanpathss64[i].scanpaths64[j].IsScanPath64 == false) {
                            pos_x = j;
                            pos_y = i;
                            breakflag = true;
                        }
                    }
                }
            }
            if (breakflag == false) {
                IsFindAll = true;
            }
        }
    }
    return 0;
}
void UpwindPrint(Clipper2Lib::Paths64& paths64) {
    for (auto& path64 : paths64) {
        std::reverse(path64.begin(), path64.end());
    }
    std::reverse(paths64.begin(), paths64.end());
}
//void UpwindPrint(ScanPathss64& scanpathss64) {
//    for (auto& paths64 : pathss64) {
//        for (auto& path64 : paths64) {
//            std::reverse(path64.begin(), path64.end());
//        }
//        std::reverse(paths64.begin(), paths64.end());
//    }
//    std::reverse(pathss64.begin(), pathss64.end());
//bool boolsortoo(const Clipper2Lib::Paths64& a, const Clipper2Lib::Paths64& b) {
//    auto result1 = std::minmax_element(a.begin(), a.end(), [](const Clipper2Lib::Path64& p1, const Clipper2Lib::Path64& p2) {return p1[0].x < p2[0].x; });
//    float center1 = (result1.first->x + result1.second->x) / 2;
//    auto result2 = std::minmax_element(b.begin(), b.end(), [](const Point2f& p1, const Point2f& p2) {return p1.x < p2.x; });
//    float center2 = (result2.first->x + result2.second->x) / 2;
//    return center1 > center2;
//}




void BoundaryUpwindPrint(std::vector<Clipper2Lib::Paths64>& paths, AirOutlet air) {
    switch (air)
    {
    case AirOutlet::Right: {
        std::sort(paths.begin(), paths.end(), [](const Clipper2Lib::Paths64& a, const Clipper2Lib::Paths64& b) {
            return a[0][0].x > b[0][0].x;
            });
        break;
    }
    case AirOutlet::Near: {
        std::sort(paths.begin(), paths.end(), [](const Clipper2Lib::Paths64& a, const Clipper2Lib::Paths64& b) {
            return a[0][0].y < b[0][0].y;
            });
        break;
    }
    case AirOutlet::Left: {
        std::sort(paths.begin(), paths.end(), [](const Clipper2Lib::Paths64& a, const Clipper2Lib::Paths64& b) {
            return a[0][0].x < b[0][0].x;
            });
        break;
    }
    case AirOutlet::Inner: {
        std::sort(paths.begin(), paths.end(), [](const Clipper2Lib::Paths64& a, const Clipper2Lib::Paths64& b) {
            return a[0][0].y > b[0][0].y;
            });
        break;
    }

    }
    return;
}

void Paths64UpWind(Clipper2Lib::Paths64& paths64, const AirOutlet air) {
    if (paths64.size() == 0)return;
    switch (air)
    {
    case AirOutlet::Right: {
        std::sort(paths64.begin(), paths64.end(), [](const Clipper2Lib::Path64& a, const Clipper2Lib::Path64& b) {
            return a[0].x > b[0].x; });
        break;
    }
    case AirOutlet::Near: {
        std::sort(paths64.begin(), paths64.end(), [](const Clipper2Lib::Path64& a, const Clipper2Lib::Path64& b) {
            return a[0].y < b[0].y; });
        break;
    }
    case AirOutlet::Left: {
        std::sort(paths64.begin(), paths64.end(), [](const Clipper2Lib::Path64& a, const Clipper2Lib::Path64& b) {
            return a[0].x < b[0].x; });
        break;
    }
    case AirOutlet::Inner: {
        std::sort(paths64.begin(), paths64.end(), [](const Clipper2Lib::Path64& a, const Clipper2Lib::Path64& b) {
            return a[0].y > b[0].y; });
        break;
    }

    }
    return;
}
void GetMarkJumpTime(const Clipper2Lib::Paths64& fill, const Clipper2Lib::Paths64& contour, const float fillspeed, const float contourspeed, double& marktime, double& jumptime) {
    double MarkDistance = 0;
    double JumpDistance = 0;
    //填充
    for (int i = 0; i < fill.size(); i++) {
        MarkDistance += DisTwoPoints(fill[i][0], fill[i][1]) / 5882.f;
    }
    for (int i = 0; i + 1 < fill.size(); i++) {
        JumpDistance += DisTwoPoints(fill[i][1], fill[i + 1][0]) / 5882.f;
    }
    marktime += MarkDistance / fillspeed;
    MarkDistance = 0;
    //轮廓
    if (fill.size() != 0 && contour.size() != 0)
        JumpDistance += DisTwoPoints(fill.back()[1], contour.front()[0]) / 5882.f;
    for (int i = 0; i < contour.size(); i++) {
        for (int j = 0; j < contour[i].size() - 1; j++) {
            MarkDistance += DisTwoPoints(contour[i][j], contour[i][j + 1]) / 5882.f;
        }
        if (i < contour.size() - 1) {
            JumpDistance += DisTwoPoints(contour[i].back(), contour[i + 1].front()) / 5882.f;
        }

    }
    marktime += MarkDistance / contourspeed;
    jumptime += JumpDistance / 3000.f;


}

void ShowPaths64(const Clipper2Lib::Paths64& paths) {
    for (auto& i : paths) {
        for (auto& j : i)
            std::cout << j.x << " " << j.y << std::endl;
        std::cout << std::endl;
    }
}

void recordContour(const Clipper2Lib::PolyPath64* treeroot, std::vector<Clipper2Lib::Paths64>& solution) {
    Paths64 region;
    region.push_back(treeroot->Polygon());

    for (int i = 0; i < treeroot->Count(); i++)
    {
        region.push_back(treeroot->Child(i)->Polygon());
        if (treeroot->Child(i)->Count() != 0)
        {
            for (int j = 0; j < treeroot->Child(i)->Count(); j++)
                recordContour(treeroot->Child(i)->Child(j), solution);
        }
    }
    solution.push_back(region);
}

void SplitBoundary(Clipper2Lib::PolyTree64& root, std::vector<Clipper2Lib::Paths64>& solution) {
    if (root.Count() == 0) return;

    for (int i = 0; i < root.Count(); i++)
    {
        recordContour(root.Child(i), solution);
    }
}
void Connect(const Clipper2Lib::Paths64& paths, Clipper2Lib::Paths64& output) {
    Clipper2Lib::Path64 path;
    for (auto& i : paths) {
        path.reserve(path.size() + i.size());
        copy(i.begin(), i.end(), back_inserter(path));
    }
    output.push_back(path);
}
inline bool GetUpWindIsLowToHigh(const double rotate_angle, const AirOutlet air) {
    //默认角度 在0~360
    switch (air)
    {
    case AirOutlet::Right: {
        if (rotate_angle < 180) { return true; }
        else { return false; }
    }
    case AirOutlet::Near: {
        if (rotate_angle < 270 && rotate_angle>90) { return false; }
        else { return true; }
    }
    case AirOutlet::Left: {
        if (rotate_angle < 180) { return false; }
        else { return true; }
    }
    case AirOutlet::Inner: {
        if (rotate_angle < 270 && rotate_angle>90) { return true; }
        else { return false; }
    }
    default:
        throw "Wind Error";
        break;
    }
}
void Paths64Planing(const CBoundary& boundary, double interval, double compensation, double  rotate_angle, const AirOutlet air, Clipper2Lib::Paths64& paths64fill, Clipper2Lib::Paths64& paths64contour) {
    CBoundary cb = boundary;
    CBoundary cbContour = boundary;
    RotateAngle(cb, rotate_angle);
    Clipper2Lib::Paths64 p = CBoundaryFloatToInt64(cb);
    p = SimplifyPaths(p, 0, true);
    Clipper2Lib::JoinType jt = Clipper2Lib::JoinType::Round;
    int ratioCompensation = compensation * 5882;
    int intervaluse = interval * 5882;
    p = InflatePaths(p, -ratioCompensation, jt, Clipper2Lib::EndType::Polygon);
    p = SimplifyPaths(p, 50, false);

    if (p.size() != 0) {
        PolyTree64 polytree;
        Clipper64 c64;
        c64.AddSubject(p);
        c64.Execute(ClipType::Union, FillRule::NonZero, polytree);

        std::vector<Clipper2Lib::Paths64> solution1;
        std::vector<Clipper2Lib::Paths64> solution2;
        SplitBoundary(polytree, solution1);

        for (int i = 0; i < solution1.size(); i++) {
            Edges edges = Paths64ToEdges(solution1[i], true);
            ScanPathss64 scanpss;
            bool IsLowToHigh = GetUpWindIsLowToHigh(rotate_angle, air);
            EdgesToScanPathsFast(edges, intervaluse, IsLowToHigh, true, scanpss);
            Clipper2Lib::Paths64 p1;
            ReduceHeliJump(scanpss, 5, true, p1);//important variable
            if (p1.size() > 0)
                solution2.push_back(p1);

        }

        RotateAngle(solution2, -rotate_angle);

        BoundaryUpwindPrint(solution2, air);
        for (const auto& i : solution2) {
            paths64fill.reserve(paths64fill.size() + i.size());
            std::copy(i.begin(), i.end(), std::back_inserter(paths64fill));
        }
    }

    //扫描轮廓 
    Clipper2Lib::Paths64 ps;
    ps = CBoundaryFloatToInt64(cbContour);
    ps = InflatePaths(ps, -ratioCompensation, jt, Clipper2Lib::EndType::Polygon);
    ps = SimplifyPaths(ps, 50, true);
    if (ps.size() != 0 && paths64fill.size() != 0) {
        Clipper2Lib::Point64 FillLastPoint = paths64fill.back()[1];
        int i_tmp = 0;
        double DistanceMin = INT64_MAX;
        for (int i = 0; i < ps[0].size(); i++) {
            int dis = DisTwoPoints(ps[0][i], FillLastPoint);
            if (dis < DistanceMin) {
                DistanceMin = dis;
                i_tmp = i;
            }

        }
        Clipper2Lib::Path64 p64;
        for (int i = i_tmp; i < i_tmp + ps[0].size(); i++) {
            int j = i % ps[0].size();
            p64.push_back(ps[0][j]);
        }
        ps[0] = p64;
        paths64contour = ps;
    }

}
void AutoFeed(double AreaPlate, double AreaEntity, double layerThickness, double& addThickness) {
    float ratio = AreaEntity / AreaPlate;
    int precision = std::round(layerThickness / 0.01);
    if (ratio > 0.5)
        addThickness = layerThickness;
    else if (ratio > 0.40)
        addThickness = std::round(precision * 0.85) * 0.01;
    else if (ratio > 0.25)
        addThickness = std::round(precision * 0.65) * 0.01;
    else if (ratio > 0.15)
        addThickness = std::round(precision * 0.45) * 0.01;
    else if (ratio > 0.1)
        addThickness = std::round(precision * 0.25) * 0.01;
    else
        addThickness = 0;
}
void ZigZagPlaning(const CBoundary& boundary, double interval, double compensation, double  rotate_angle, const AirOutlet air, Clipper2Lib::Paths64& paths64fill, Clipper2Lib::Paths64& paths64contour) {
    Clipper2Lib::Paths64 p = CBoundaryFloatToInt64(boundary);
    p = SimplifyPaths(p, 10, true);
    Clipper2Lib::JoinType jt = Clipper2Lib::JoinType::Round;
    double ratioCompensation = compensation * 5882;
    int intervaluse = interval * 5882;
    p = InflatePaths(p, -ratioCompensation, jt, Clipper2Lib::EndType::Polygon);
    p = SimplifyPaths(p, 50, true);
    paths64contour = p;

    RotateAngle(p, rotate_angle);
    if (p.size() != 0) {
        PolyTree64 polytree;
        Clipper64 c64;
        c64.AddSubject(p);
        c64.Execute(ClipType::Union, FillRule::NonZero, polytree);

        std::vector<Clipper2Lib::Paths64> solution1;
        std::vector<Clipper2Lib::Paths64> solution2;
        SplitBoundary(polytree, solution1);
        for (int i = 0; i < solution1.size(); i++) {
            Edges edges = Paths64ToEdges(solution1[i], true);
            Clipper2Lib::Paths64 pss;
            bool IsLowToHigh = GetUpWindIsLowToHigh(rotate_angle, air);
            EdgesToScanPaths(edges, intervaluse, IsLowToHigh, true, pss);
            if (pss.size() > 0)
                solution2.push_back(pss);
        }
        RotateAngle(solution2, -rotate_angle);
        BoundaryUpwindPrint(solution2, air);
        for (const auto& i : solution2) {
            paths64fill.reserve(paths64fill.size() + i.size());
            std::copy(i.begin(), i.end(), std::back_inserter(paths64fill));
        }
    }
}

void RasterPlaning(const CBoundary& boundary, double interval, double compensation, double  rotate_angle, const AirOutlet air, Clipper2Lib::Paths64& paths64fill, Clipper2Lib::Paths64& paths64contour) {
    Clipper2Lib::Paths64 p = CBoundaryFloatToInt64(boundary);
    p = SimplifyPaths(p, 10, true);
    Clipper2Lib::JoinType jt = Clipper2Lib::JoinType::Round;
    double ratioCompensation = compensation * 5882;
    int intervaluse = interval * 5882;
    p = InflatePaths(p, -ratioCompensation, jt, Clipper2Lib::EndType::Polygon);
    p = SimplifyPaths(p, 50, true);
    paths64contour = p;

    RotateAngle(p, rotate_angle);
    if (p.size() != 0) {
        PolyTree64 polytree;
        Clipper64 c64;
        c64.AddSubject(p);
        c64.Execute(ClipType::Union, FillRule::NonZero, polytree);

        std::vector<Clipper2Lib::Paths64> solution1;
        std::vector<Clipper2Lib::Paths64> solution2;
        SplitBoundary(polytree, solution1);
        for (int i = 0; i < solution1.size(); i++) {
            Edges edges = Paths64ToEdges(solution1[i], true);
            Clipper2Lib::Paths64 pss;
            EdgesToRaster(edges, intervaluse, true, true, pss);
            if (rotate_angle > 180)
                UpwindPrint(pss);
            if (pss.size() > 0)
                solution2.push_back(pss);
        }
        RotateAngle(solution2, -rotate_angle);
        BoundaryUpwindPrint(solution2, air);
        for (const auto& i : solution2) {
            paths64fill.reserve(paths64fill.size() + i.size());
            std::copy(i.begin(), i.end(), std::back_inserter(paths64fill));
        }
    }
}
void ShowSvg(const Clipper2Lib::Paths64& fill, const Clipper2Lib::Paths64& contour) {


    SvgWriter svg;//改色修改pencolor
    svg.AddPaths(contour, false, FillRule::Negative, 0x00000000, 0xFFFF0000, 1.3, false);
    FillRule fr = FillRule::Negative;
    SvgAddOpenSubject(svg, fill, fr, false);
    SvgSaveToFile(svg, "1.svg", 900, 1800, 20);
    System("1.svg");
}

static inline glm::vec4 GetColor(float cur, float min, float max) {
    ImColor color; color.SetHSV(0.7 - 0.7 * SlowInterpolation((cur - min) / (max - min)), 1.f, 1.f);
    glm::vec4 color1 = { color.Value.x,color.Value.y,color.Value.z,1.f };
    return color1;
}
void ShowScanLinesSVG(const ScanLines& lines) {
    SvgWriter svg;//改色修改pencolor
   //
    FillRule fr = FillRule::Negative;
    for (auto& i : lines) {
        unsigned int  r = GetColor(i.power, 50, 125).r*255;
        unsigned int  g = GetColor(i.power, 50, 125).g*255;
        unsigned int  b = GetColor(i.power, 50, 125).b*255;
        unsigned int color = (0xFF << 24) + (r << 16) + (g << 8) + b;
        svg.AddPath(i.path64, true, FillRule::Negative, 0x00000000, color, 2.5, false);
    }
    //SvgAddOpenSubject(svg, fill, fr, false);
    SvgSaveToFile(svg, "1.svg", 900, 1800, 20);
    System("1.svg");
}
void Paths64RatioConvert(Clipper2Lib::Paths64& path, double ratio) {
    for (auto& i : path) {
        for (auto& j : i) {
            int64_t x = std::round(j.x * ratio);
            int64_t y = std::round(j.y * ratio);
            j.x = x;
            j.y = y;
        }
    }
}
int NeedUpDownSkinDeal(const CLayers& clayers, int layer_index, const int num_layers, const float toleranceUp, const float toleranceDown) {
    if (clayers.size() <= num_layers)return -1;
    if (layer_index <= num_layers)return 1;
    if (layer_index >= (clayers.size() - num_layers))return 1;
    if (layer_index >= num_layers) {
        //判断上表面
        Clipper2Lib::Paths64 current_layer = CBoundaryFloatToInt64(clayers[layer_index].bound);
        Clipper2Lib::Paths64 next_layer = CBoundaryFloatToInt64(clayers[layer_index + num_layers].bound);
        Clipper2Lib::Paths64 deltaUp = Difference(current_layer, next_layer, FillRule::NonZero);

        double origin_area = abs(Clipper2Lib::Area(current_layer));
        double delta_area = abs(Clipper2Lib::Area(deltaUp));
        if ((delta_area / origin_area) > toleranceUp)
            return 1;
        //判断下表面
        Clipper2Lib::Paths64 last_layer = CBoundaryFloatToInt64(clayers[layer_index - num_layers].bound);
        Clipper2Lib::Paths64 deltaDown = Difference(current_layer, last_layer, FillRule::NonZero);
        delta_area = abs(Clipper2Lib::Area(deltaDown));
        if ((delta_area / origin_area) > toleranceDown)
            return 2;
    }
    return 0;
}
void VariPathssConvert(VariPathss& vpss, double ratio) {
    for (auto& i : vpss) {
        i.JumpPoint = i.JumpPoint * ratio;
        for (auto& j : i.MarkPoint) {
            j.point = j.point * ratio;
        }
    }
}
Cube2 GerateCube2(const Clipper2Lib::Paths64& cb) {
    int64_t xmin = INT32_MAX;
    int64_t xmax = INT32_MIN;
    int64_t ymin = INT32_MAX;
    int64_t ymax = INT32_MIN;

    for (const auto& i : cb) {
        for (const auto& point : i) {
            if (point.x < xmin)
                xmin = point.x;
            if (point.x > xmax)
                xmax = point.x;
            if (point.y < ymin)
                ymin = point.y;
            if (point.y > ymax)
                ymax = point.y;
        }
    }
    Cube2 cube = { {xmin,ymin},{xmax,ymax} };
    return cube;
}

Clipper2Lib::Paths64 MakeRectStrips(const Cube2& cube, const int stripWidth, const int tolerance, const bool IsLowToHigh) {
    if (tolerance > stripWidth || (cube.Max.y - cube.Min.y) <= stripWidth)
        assert("out of range");
    int ny = ceil((double)(cube.Max.y - cube.Min.y) / stripWidth);
    Clipper2Lib::Paths64 psd;
    for (int i = 1; i <= ny; i++) {
        Clipper2Lib::Path64 pd;
        if (i == 1) {
            pd.push_back({ cube.Min.x - tolerance,cube.Min.y - tolerance });//左下启，逆时针
            pd.push_back({ cube.Max.x + tolerance,cube.Min.y - tolerance });
            
            pd.push_back({ cube.Max.x + tolerance,cube.Min.y + i * stripWidth + tolerance });
            pd.push_back({ cube.Min.x - tolerance,cube.Min.y + i * stripWidth + tolerance });
        }
        else {
            pd.push_back({ cube.Min.x - tolerance,cube.Min.y + (i - 1 )* stripWidth });//左下启，逆时针
            pd.push_back({ cube.Max.x + tolerance,cube.Min.y + (i - 1 )* stripWidth });
            
            pd.push_back({ cube.Max.x + tolerance,cube.Min.y + (i) * stripWidth + tolerance });
            pd.push_back({ cube.Min.x - tolerance,cube.Min.y + (i) * stripWidth + tolerance });
        }
        psd.push_back(pd);
    }
    if (IsLowToHigh == false)
        std::reverse(psd.begin(), psd.end());
    return psd;
}

//Clipper2Lib::Paths64 StripPaths(const Clipper2Lib::Paths64 &subject,const Clipper2Lib::Paths64&clips,int interval, double angle)
//{
//    Clipper2Lib::Paths64 paths;
//    for (const auto& clip : clips) {
//        Clipper2Lib::Paths64 cp;
//        cp.emplace_back(clip);
//        Clipper2Lib::Paths64 tmp = Difference(subject, cp, Clipper2Lib::FillRule::NonZero);
//        Edges edges = Paths64ToEdges(tmp, true);
//        Clipper2Lib::Paths64 pss;
//        EdgesToScanPaths(edges, interval, , true, pss);
//    }
//    return rectclips;
//
//}
void StripPlaning(const CBoundary& boundary, double interval, double compensation, double  rotate_angle, const AirOutlet air,
    double stripwidth, double tolerance, Clipper2Lib::Paths64& paths64fill, std::vector<Clipper2Lib::Paths64>& paths64contour,int BoundaryOffsetTimes)
{
    CBoundary cbContour = boundary;
    Clipper2Lib::Paths64 p = CBoundaryFloatToInt64(boundary);
    p = SimplifyPaths(p, 10, false);
    Clipper2Lib::JoinType jt = Clipper2Lib::JoinType::Square;
    double ratioCompensation = compensation * 5882;
    int intervaluse = interval * 5882;
    for (int i = 0; i < BoundaryOffsetTimes; i++) {
        p = InflatePaths(p, -ratioCompensation, jt, Clipper2Lib::EndType::Polygon);
        p = SimplifyPaths(p, 50, true);
        paths64contour.emplace_back(p);
    }
    std::reverse(paths64contour.begin(), paths64contour.end());

    p = InflatePaths(p, -ratioCompensation, jt, Clipper2Lib::EndType::Polygon);
    p = SimplifyPaths(p, 50, true);
    //paths64contour[0].erase(std::remove_if(paths64contour[0].begin(), paths64contour[0].end(), [](Clipper2Lib::Path64 a) { return a.size() > 10; }), paths64contour[0].end());

    
    if (p.size() != 0) {
        PolyTree64 polytree;
        Clipper64 c64;
        c64.AddSubject(p);
        c64.Execute(ClipType::Union, FillRule::NonZero, polytree);

        std::vector<Clipper2Lib::Paths64> solution1,solution2;
        SplitBoundary(polytree, solution1);
        BoundaryUpwindPrint(solution1, air);
        RotateAngle(solution1, rotate_angle);
        //轮廓环逆风打印
        for (int i = 0; i < solution1.size(); i++) {
            Cube2 cube = GerateCube2(solution1[i]);
            bool IsLowToHigh = GetUpWindIsLowToHigh(rotate_angle, air);
            Clipper2Lib::Paths64 rects = MakeRectStrips(cube, stripwidth * 5882, tolerance * 5882, IsLowToHigh);
            //切割
            for (const auto& clip : rects) {
                Clipper2Lib::Paths64 cp;
                cp.emplace_back(clip);
                Clipper2Lib::Paths64 rings = Intersect(solution1[i], cp, Clipper2Lib::FillRule::NonZero);
                Edges edges = Paths64ToEdges(rings, false);
                Clipper2Lib::Paths64 pss;
                int angle = int(rotate_angle-90+360) % 360;
                bool IsLefttoRight = GetUpWindIsLowToHigh(angle, air);
                EdgesToScanPaths(edges, intervaluse, IsLefttoRight, false, pss);
                paths64fill.reserve(paths64fill.size() + pss.size());
                std::copy(pss.begin(), pss.end(), std::back_inserter(paths64fill));
            }
        }
        RotateAngle(paths64fill, -rotate_angle);
    }
    
}