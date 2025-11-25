#pragma once
#include<atomic>
#include "Rtc5Scan.h"

enum ScanMode {
	光栅扫描 = 0,
	Zigzag,
	最短空行程,
	线状支撑,
	块状支撑,
	卷积核,
	条带路径
};
const std::string scanModeStrings[] = {
	"光栅扫描",
	"Zigzag",
	"最短空行程",
	"线状支撑",
	"块状支撑",
	"卷积核",
	"条带扫描"
}; 

// 将枚举值转换为字符串
inline std::string enumToString(ScanMode mode) {
	return scanModeStrings[mode];
}
struct PartVarible {
	ScanMode scanmode = 最短空行程;
	struct {
		double 间隙=0.08;
		double 旋转角度=67;
		double 功率=120;
		double 速度=1000;
	}Fill;
	struct {
		bool IsContour = true;
		int 偏置次数 = 1;
		double 功率 = 135;
		double 速度 = 450;
		double 功率低 = 100;
	}Contour;
	struct {
		bool IsUpSkin = false;
		double 间隙 = 0.08;
		double 旋转角度 = 0;
		double 功率 = 120;
		double 速度 = 1000;
		int 上表面层号 = 286;
		int 层数 = 3;
		int 上表面层号2 = 36;//temp
		int 层数2 = 3;

		bool IsMelt = false;
		double 重熔间隙 = 0.08;
		double 重熔旋转角度 = 0;
		double 重熔功率 = 120;
		double 重熔速度 = 1000;
		int 重熔次数 = 1;
	}Upskin;
	struct {
		double 间隙 = 0.08;
		double 旋转角度 = 0;
		double 功率 = 120;
		double 速度 = 1000;
	}Downskin;
	struct {
		double 间隙 = 0.08;
		double 旋转角度 = 67;
		double 速度 = 1000;
		double power_high=160;
		double power_low = 140;
		double precision = 0.1;
		int num_xy =5;
		int num_z = 12;
		double  scanLenth=0.1;
	}Kernel;
	struct {
		double 条带宽度 = 20;
		double 延伸长度 = 0.1;
		//double 间隙 = 0.08;
		//double 激光功率 = 120;
		//double 扫描速度 = 1000;
		//double 旋转角度 = 67;
	}Strip;
};
struct ScanParamter {
	struct {
		double 打印层厚;
		double 光斑半径补偿;
		double 氧含量上限;
		double 腔体压力上限;
		double 风机压力;
	}Gobal;
	std::vector<PartVarible> Part;
	struct {
		int LaserMode;
		int MarkSpeed;
		int JumpDelay;
		int MarkDelay;
		int PolygonDelay;
		int LaserOnDelay;
		int LaserOffDelay;
	}Laser;

};
//extern std::atomic<bool>ProcessFlag;
//void Manufacturing(CLayers clayers, float interval, double rotate_angle, float Z_Delta);
void MultiPartsPrint(ScanParamter scanparameter, std::vector<CLayers> clayerss, bool IsStimulate, std::atomic<bool>& ProcessFlag);
//void OxygenControl(float oxygenRatio, int charmberPressure);
void ScanCircles(int nums_x, int nums_y, float interval_x, float interval_y, float radius, float power, float speed);
void ScanLiness(int nums_x, int nums_y, float interval_x, float interval_y, float radius, float power, float speed);
void MarkCircles(int nums_x, int nums_y, float interval_x, float interval_y, float radius, float tolerance, float power, float speed,bool delay);

void Love(double radius, int sampleTimes, float power, float speed);
ScanLines LoveScanLines(double radius, int sampleTimes, float power, float speed);
void ArchimedeanSpirals(double radius, double interval, int sampleTimes, float power, float speed);
ScanLines ArchimedeanSpiralsScanLines(double radius, double interval, int sampleTimes, float power, float speed);
void DynamicRectLines(double length, double interval_y, int nums_x, int nums_y, double power, double speed);
ScanLines DynamicRectLinesScanLines(double length, double interval_y, int nums_x, int nums_y, double power, double speed);
void Bitmap(int resolution_x, int resolution_y, double dot_size, float power, float PixelPeriod = 200);
ScanLines BitmapScanLines(int resolution_x, int resolution_y, double dot_size, float power, float PixelPeriod);
void ScanCrossLines(int nums_x, int nums_y, float interval_x, float interval_y, float length, float power, float speed);



