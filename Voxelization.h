#pragma once
#include<limits>
#include"CustomDataStructure.h"
#include "clipper2.h"
template<class Number>
using Point3 = glm::tvec3<Number>;
typedef Point3<double> Point3d;
typedef Point3<float> Point3f;
template <class Number>
struct Cube
{
public:
	//data field
	Point3<Number> min, max;

	//constructor
	Cube(void) { reset(); };

	//operation for setting min/max from serialise points
	void dealPoint(Point3<Number> p);

	//method
	void constexpr reset(void);	//重置为未初始化状态，即刚刚构造的时候无cube空间的状态
	//inline void zerolize(void) { min.x = min.y = min.z = max.x = max.y = max.z = (Number)0; };
	inline void shift(Point3<Number> p_shift) { min += p_shift; max += p_shift; };

	//property
	bool bInit(void); //cube是否被初始化过，即是否至少运行过一次dealPoint()

	inline Number w(void) const { return max.x - min.x; };
	inline Number h(void) const { return max.y - min.y; };
	inline Number t(void) const { return max.z - min.z; };
	inline Point3<Number> dim(void) const { return Point3<Number>{w(), h(), t()}; };
	inline Number maxDim(void) const { return std::max({ w(), h(), t() }); };
	inline Number maxLength(void) const { return (Number)sqrt(w() * w() + h() * h() + t() * t()); };

	inline Number midX(void) const { return (min.x + max.x) / 2; };
	inline Number midY(void) const { return (min.y + max.y) / 2; };
	inline Number midZ(void) const { return (min.z + max.z) / 2; };
	inline Point2<Number> midXY(void) const { return Point2<Number>(midX(), midY()); };
	inline Point3<Number> mid(void) const { return Point3<Number>(midX(), midY(), midZ()); };
};


template <class T>
inline void setMin(T x, T& min) {
	if (x < min) min = x;
};

template <class T>
inline void setMax(T x, T& max) {
	if (x > max) max = x;
};

template <class T>
inline void setMinMax(T x, T& min, T& max) {
	setMin(x, min);
	setMax(x, max); 
};
template<class Number>
inline constexpr void Cube<Number>::reset(void)
{
	const Number nMax = std::numeric_limits<Number>::max();
	min = { nMax, nMax, nMax };
	const Number nLowest = std::numeric_limits<Number>::lowest();
	max = { nLowest, nLowest, nLowest };
}

template <class Number>
inline bool Cube<Number>::bInit(void)
{
	return min.x < std::numeric_limits<Number>::max();
};

template <class Number>
void Cube<Number>::dealPoint(Point3<Number> p)
{
	setMinMax(p.x, min.x, max.x);
	setMinMax(p.y, min.y, max.y);
	setMinMax(p.z, min.z, max.z);
}



typedef Cube<double> CubeD;
typedef std::vector<std::vector<unsigned char>> Voxel;
typedef std::vector<std::vector<float>> VoxelFactor;
struct VoxelDimension {
	double X_begin; //起点X
	double Y_begin;//起点Y
	double precision = 1;//体素精度
	int n_x;//X方向个数
	int n_y;//Y方向个数
	int num_margin = 5;//边缘扩展个数
};
struct Point2ID {
	int dx;
	int dy;
	float d;
};

struct KerneL{
	int partId;
	CubeD cube;
	VoxelDimension voxelDim;
	std::vector<std::vector<Point2ID>> pointID;
	FixedSizeQueue<Voxel> queue;
	int standardFactor;
	
};

struct RGBA {
	unsigned char r, g, b, a;
};
//
CubeD GetCubf(const CLayers& clayers);
void MakeVoxelDimension(const CubeD& cube , VoxelDimension& voxelDis);
double distanceFromOrigin(int i, int j, int k, double precision);
Voxel ExecVoxelization(const CBoundary& contour, const VoxelDimension& voxelDis);
std::vector<std::vector<Point2ID>> makeSquereID(const int sizeR, const int sizeZ, float precision, float thickness);
float Kernel(const FixedSizeQueue<Voxel>& queue, const std::vector<std::vector<Point2ID>> IDs, const int index_x, const int index_y);
float StandardFactor(const std::vector<std::vector<Point2ID>> IDs);
VoxelFactor CaculateFactor(const CBoundary& contour, const int layer_index, const VoxelDimension& voxelDis
	, FixedSizeQueue<Voxel>& queue, const std::vector<std::vector<Point2ID>>& pointID, const float standardfactor);
VariPathss  GeneratePaths(const Clipper2Lib::Paths64& path, const VoxelFactor& vf
	, const VoxelDimension& voxelDis, const float scanlength, float power_high, float power_low);

VariPathss  GeneratePathsContour(const Clipper2Lib::Paths64& path, const VoxelFactor& vf
	, const VoxelDimension& voxelDis, const float scanlength, float power_high, float power_low);

void ShowVariPathss(const VariPathss& vpss, float power_high, float power_low);