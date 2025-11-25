#pragma once
#include <cstdlib>
#include<functional>
#include "clipper2/clipper.h"
#include "clipper.svg.h"
#include "clipper.svg.utils.h"


#include "slcReader.h"

static const float galvanometerRatio = 5882.f;
static const float nextMachine = 375.f;

enum AirOutlet {
	Right = 0,
	Near,
	Left,
	Inner
};

//定义边
struct Edge {
	int64_t ymin;
	int64_t x_ymin;
	int64_t ymax;
	double m;// m = 1 / slope
};
typedef std::vector <Edge> Edges;
const float PI_180 = 57.29578;

//定义扫描路径
struct ScanPath64 {
	Clipper2Lib::Path64 path64;
	bool IsScanPath64 = false;
};
struct ScanPaths64 {
	std::vector<ScanPath64> scanpaths64;
	bool IsScanPaths64 = false;
};

typedef std::vector<ScanPaths64> ScanPathss64;


struct VariPath {
	Clipper2Lib::Point64 point;
	float power;
	//float speed;
};
struct VariPaths {
	Clipper2Lib::Point64 JumpPoint;
	std::vector<VariPath> MarkPoint;
};
using VariPathss = std::vector< VariPaths>;

//包围盒
struct Cube2 {
	Clipper2Lib::Point64 Min, Max;
};

template<typename  T>
inline double DisTwoPoints(const T& p1, const T& p2) {
	return std::sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}
struct ScanLine {
	Clipper2Lib::Path64 path64;
	float power;
	float speed;
	float delay;//ms
	ScanLine(Clipper2Lib::Path64 path64, float power, float speed) :path64(path64), power(power), speed(speed), delay(0.f) {};
	ScanLine(Clipper2Lib::Path64 path64, float power, float speed, float delay) :path64(path64), power(power), speed(speed), delay(delay) {};
};
using ScanLines = std::vector<ScanLine>;

void Paths64RatioConvert(Clipper2Lib::Paths64& path, double ratio);
void VariPathssConvert(VariPathss& vpss, double ratio);

void RotateAngle(Clipper2Lib::Paths64& paths64, double angle);
void RotateAngle(std::vector<Clipper2Lib::Paths64>& paths64, double angle);
void RotateAngle(ScanPathss64& scanpathss64, double angle);
void RotateAngle(CBoundary& Cboundary, double angle);




Clipper2Lib::Paths64 CBoundaryFloatToInt64(const CBoundary& Cboundary);
void SplitBoundary(Clipper2Lib::PolyTree64& root, std::vector<Clipper2Lib::Paths64>& solution);
Edges Paths64ToEdges(Clipper2Lib::Paths64& paths64, bool IsScanLineSLevel);
void EdgesToScanPaths(Edges& edges, int64_t interval, bool IsLowToHigh, bool IsScanLineSLevel, Clipper2Lib::Paths64& paths64);
void EdgesToRaster(Edges& edges, int64_t interval, bool IsLowToHigh, bool IsScanLineSLevel, Clipper2Lib::Paths64& paths64);
//返回scanpathss64的线段总条数
int EdgesToScanPathsFast(Edges& edges, int64_t interval, bool IsLowToHigh, bool IsScanLineSLevel, ScanPathss64& scanpathss64);



void System(const std::string& filename);
void BeamCompensation(Clipper2Lib::Paths64& paths64, int beamRadius);


void UpwindPrint(Clipper2Lib::Paths64& paths64);
void UpwindPrint(ScanPathss64& scanpathss64);
void BoundaryUpwindPrint(std::vector<Clipper2Lib::Paths64>& paths, AirOutlet air);
void Paths64UpWind(Clipper2Lib::Paths64& paths64, const AirOutlet air);

int NeedUpDownSkinDeal(const CLayers& clayers, int layer_index, const int num_layers, const float toleranceUp, const float toleranceDown);

int ReduceHeliJump(Clipper2Lib::Paths64& paths64, Clipper2Lib::Paths64& temp);
int ReduceHeliJump(ScanPathss64& scanpathss64, const float tolerance_x, const int tolerance_y, Clipper2Lib::Paths64& output);
int ReduceHeliJump(ScanPathss64& scanpathss64, const float tolerance_x, bool IsAbusolute, Clipper2Lib::Paths64& output);
//CBoundary CBoundaryInt64ToFloat(Clipper2Lib::Paths64 paths64 );

//tool4
void GetMarkJumpTime(const Clipper2Lib::Paths64& fill, const Clipper2Lib::Paths64& contour, const float fillspeed, const float contourspeed, double& marktime, double& jumptime);
void ShowPaths64(const Clipper2Lib::Paths64& paths);
void Connect(const Clipper2Lib::Paths64& paths, Clipper2Lib::Paths64& output);
void ShowSvg(const Clipper2Lib::Paths64& fill, const Clipper2Lib::Paths64& contour);
void ShowScanLinesSVG(const ScanLines& lines);
//route
void AutoFeed(double AreaPlate, double AreaEntity, double layerThickness, double& addThickness);
//这里的boundary会被修改
void Paths64Planing(const CBoundary& boundary, double interval, double compensation, double  rotate_angle, const AirOutlet air, Clipper2Lib::Paths64& paths64fill, Clipper2Lib::Paths64& paths64contour);
void ZigZagPlaning(const CBoundary& boundary, double interval, double compensation, double  rotate_angle, const AirOutlet air, Clipper2Lib::Paths64& paths64fill, Clipper2Lib::Paths64& paths64contour);
void RasterPlaning(const CBoundary& boundary, double interval, double compensation, double  rotate_angle, const AirOutlet air, Clipper2Lib::Paths64& paths64fill, Clipper2Lib::Paths64& paths64contour);
void StripPlaning(const CBoundary& boundary, double interval, double compensation, double  rotate_angle, const AirOutlet air,
	double stripwidth, double tolerance, Clipper2Lib::Paths64& paths64fill, std::vector<Clipper2Lib::Paths64>& paths64contour, int BoundaryOffsetTimes = 1);


inline Clipper2Lib::Paths64 ConnectPloygon(const Clipper2Lib::Paths64& p) {
	Clipper2Lib::Paths64 ret;
	for (auto& i : p) {
		Clipper2Lib::Path64 path;
		for (auto& j : i) {
			path.push_back(j);
		}
		path.push_back(i.front());
		ret.push_back(path);
	}

	return ret;
}

inline void LineCompensation( Clipper2Lib::Paths64& p,int64_t length) {
	for (auto& i : p) {
		i[0].x = i[0].x + length;
		i[1].x = i[1].x - length;
	}
}


inline float SlowInterpolation(float fp) { return -fp * fp + 2 * fp; }

