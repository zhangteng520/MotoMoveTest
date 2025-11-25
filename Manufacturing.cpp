#pragma once

#include<fstream>
#include<map>
#include<chrono>
#include <omp.h>
#include <math.h>

#include"Manufacturing.h"
#include"Voxelization.h"
#include"control.h"


void ScanSingle(float power, float speed);
void MultiPartsPrint(ScanParamter scanparameter, std::vector<CLayers> clayerss, bool IsStimulate, std::atomic<bool>&ProcessFlag) {
	if (!IsStimulate) {
		Axis_Move(C, 0);
		scanInitial();
		Axis_Move(C, 311);
	}
	std::ofstream outputFile2("MarkAndJump3.csv");
	if (!outputFile2)
		std::cout << "Fail to open file. " << std::endl;
	if (IsStimulate) {
		outputFile2 << "Layers" << "," << "MarkTime" << "," << "JumpTime" << std::endl;
	}
	std::ofstream outputFile("process9.csv");
	if (!outputFile)
		std::cout << "Fail to open file. " << std::endl;
	outputFile << "Layers" << "," << "scan" << "," << "Move" << std::endl;


	//找出最高处
	double Zmin_Max = 0;
	for (int i = 0; i < clayerss.size(); i++) {
		if (clayerss[i].back().zmin >= Zmin_Max)
			Zmin_Max = clayerss[i].back().zmin;
	}

	//找出最低处
	double Zmin = INT32_MAX;
	for (int i = 0; i < clayerss.size(); i++) {
		if (clayerss[i].front().zmin < Zmin)
			Zmin = clayerss[i].front().zmin;
	}
	int ZLayersMax = std::round((Zmin_Max - Zmin) / scanparameter.Gobal.打印层厚);
	double Z_Current = Zmin + 1e-4;
	//检查是否使用卷积核
	std::map<int, KerneL> kernels;
	for (int i = 0; i < clayerss.size(); i++) {
		if (scanparameter.Part[i].scanmode == ScanMode::卷积核) {
			CubeD cube = GetCubf(clayerss[i]);
			VoxelDimension voxelDim;
			voxelDim.num_margin = scanparameter.Part[i].Kernel.num_xy;
			voxelDim.precision = scanparameter.Part[i].Kernel.precision;
			MakeVoxelDimension(cube, voxelDim);
			std::vector<std::vector<Point2ID>> pointID = makeSquereID(scanparameter.Part[i].Kernel.num_xy, scanparameter.Part[i].Kernel.num_z, scanparameter.Part[i].Kernel.precision, scanparameter.Gobal.打印层厚);
			FixedSizeQueue<Voxel> queue(scanparameter.Part[i].Kernel.num_z);
			for (int j = 0; j < scanparameter.Part[i].Kernel.num_z; j++) {
				Voxel data(voxelDim.n_y, std::vector<unsigned char>(voxelDim.n_x, 1));
				queue.push(data);
			}
			int standardFactor = StandardFactor(pointID);
			KerneL ker = { i,cube,voxelDim,pointID,queue,standardFactor };
			kernels.insert(std::make_pair(i, ker));
		}
	}
	//开始加工
	double timeTotal = 0;
//	if (IsStimulate) {
//#pragma omp parallel for reduction(+:timeTotal)
//	}

	for (int layer_Gobal_index = 0; layer_Gobal_index <= ZLayersMax; layer_Gobal_index++) {
		if (ProcessFlag.load(std::memory_order_acquire) == false)
			break;

		double marktime = 0;
		double jumptime = 0;

		double entityArea = 0;
		auto startTime1 = std::chrono::steady_clock::now();
		Z_Current += scanparameter.Gobal.打印层厚;
		for (int partnum = 0; partnum < clayerss.size(); partnum++) {
			if ((Z_Current - 1e-3) > clayerss[partnum].back().zmin
				|| (Z_Current) < clayerss[partnum].front().zmin)
				continue;
			//计算零件的层号
			const auto& para = scanparameter.Part[partnum];
			int layer_index = std::round((Z_Current - clayerss[partnum].front().zmin) / scanparameter.Gobal.打印层厚);
			//扫描零件
			if (para.Upskin.IsUpSkin) {
				volatile int flag = NeedUpDownSkinDeal(clayerss[partnum], layer_index, scanparameter.Part[partnum].Upskin.层数, 0.2, 0.2);
				if (flag!=0) {
					int a = scanparameter.Part[partnum].Fill.旋转角度;
					double rotate = (a * layer_index + (int)scanparameter.Part[partnum].Upskin.旋转角度) % 360;
					Clipper2Lib::Paths64 fill, contour;
					ZigZagPlaning(clayerss[partnum][layer_index].bound, scanparameter.Part[partnum].Upskin.间隙,
						scanparameter.Gobal.光斑半径补偿, rotate,AirOutlet::Right, fill, contour);
					scanFill(fill, scanparameter.Part[partnum].Upskin.功率, scanparameter.Part[partnum].Upskin.速度);
					if (contour.size() != 0 && scanparameter.Part[partnum].Contour.IsContour == true) {
						std::sort(contour.begin(), contour.end(), [](const Clipper2Lib::Path64& a, const Clipper2Lib::Path64& b) {
							return a[0].x > b[0].x; });
						scanContour(contour, scanparameter.Part[partnum].Contour.功率, scanparameter.Part[partnum].Contour.速度);
						if (IsStimulate) {
							GetMarkJumpTime(fill, contour, para.Fill.速度, para.Contour.速度, marktime, jumptime);
						}
						//ShowSvg(fill, contour);
					}
					if (scanparameter.Part[partnum].Upskin.IsMelt == true) {
						for (int i = 0; i < scanparameter.Part[partnum].Upskin.重熔次数; i++) {
							Clipper2Lib::Paths64 fill1, contour1;
							double rotateMelt = (a * layer_index + (int)scanparameter.Part[partnum].Upskin.旋转角度
								+ (int)scanparameter.Part[partnum].Upskin.重熔旋转角度 * i) % 360;
							ZigZagPlaning(clayerss[partnum][layer_index].bound, scanparameter.Part[partnum].Upskin.重熔间隙,
								scanparameter.Gobal.光斑半径补偿, rotateMelt, AirOutlet::Right, fill1, contour1);
							scanFill(fill1, scanparameter.Part[partnum].Upskin.重熔功率, scanparameter.Part[partnum].Upskin.重熔速度);
							if (IsStimulate && fill.size() != 0 && contour.size() != 0) {
								GetMarkJumpTime(fill, contour, para.Fill.速度, para.Contour.速度, marktime, jumptime);
							}
						}
					}
					continue;
				}
			}
			if (scanparameter.Part[partnum].scanmode == ScanMode::最短空行程
				|| scanparameter.Part[partnum].scanmode == ScanMode::块状支撑) {
				Clipper2Lib::Paths64 fill, contour;
				int a = scanparameter.Part[partnum].Fill.旋转角度;
				double rotate = (a * layer_index) % 360;
				ZigZagPlaning(clayerss[partnum][layer_index].bound, scanparameter.Part[partnum].Fill.间隙,
					scanparameter.Gobal.光斑半径补偿, rotate, AirOutlet::Right,fill, contour);
				if(fill.size()!=0)
					scanFill(fill, scanparameter.Part[partnum].Fill.功率, scanparameter.Part[partnum].Fill.速度);
				if (contour.size() != 0
					&& scanparameter.Part[partnum].scanmode != ScanMode::块状支撑
					&& scanparameter.Part[partnum].Contour.IsContour == true) {
					std::sort(contour.begin(), contour.end(), [](const Clipper2Lib::Path64& a, const Clipper2Lib::Path64& b) {
						return a[0].x > b[0].x; });
					scanContour(contour, scanparameter.Part[partnum].Contour.功率, scanparameter.Part[partnum].Contour.速度);
					if (IsStimulate&&fill.size()!=0&&contour.size()!=0) {
						GetMarkJumpTime(fill, contour, para.Fill.速度, para.Contour.速度, marktime, jumptime);
					}
				}
				entityArea += abs(Clipper2Lib::Area(contour) / 5882.f / 5882.f);
			}

			else if (scanparameter.Part[partnum].scanmode == ScanMode::条带路径) {
				std::vector< Clipper2Lib::Paths64>contour;
				Clipper2Lib::Paths64 fill,tmp;
				int a = scanparameter.Part[partnum].Fill.旋转角度;
				double rotate = (a * layer_index) % 360;
				StripPlaning(clayerss[partnum][layer_index].bound, scanparameter.Part[partnum].Fill.间隙,
					scanparameter.Gobal.光斑半径补偿, rotate, AirOutlet::Right,para.Strip.条带宽度,para.Strip.延伸长度, fill, contour,para.Contour.偏置次数);
				if (fill.size() != 0)
					scanFill(fill, scanparameter.Part[partnum].Fill.功率, scanparameter.Part[partnum].Fill.速度);
				if (contour.size() != 0
					&& scanparameter.Part[partnum].Contour.IsContour == true) {
					std::sort(contour[0].begin(), contour[0].end(), [](const Clipper2Lib::Path64& a, const Clipper2Lib::Path64& b) {
						return a[0].x > b[0].x; });
					if (contour.size() == 1)
						scanContour(contour[0], para.Contour.功率, para.Contour.速度);
					else{
						for (int i = 0; i < contour.size(); i++) {
							float powerCuurnet = para.Contour.功率低 + (para.Contour.功率 - para.Contour.功率低) / (contour.size()-1) * i;
							scanContour(contour[i], powerCuurnet, para.Contour.速度);
						}
					}if (IsStimulate && fill.size() != 0 && contour.size() != 0) {
						GetMarkJumpTime(fill, contour[0], para.Fill.速度, para.Contour.速度, marktime, jumptime);
					}
				}
				entityArea += abs(Clipper2Lib::Area(contour[0]) / 5882.f / 5882.f);
			}

			else if (scanparameter.Part[partnum].scanmode == ScanMode::卷积核) {
				std::vector<Clipper2Lib::Paths64> contour;
				Clipper2Lib::Paths64 fill;
				int a = scanparameter.Part[partnum].Kernel.旋转角度;
				double rotate = (a * layer_index) % 360;
				KerneL& ker = kernels[partnum];
				VoxelFactor vf = CaculateFactor(clayerss[partnum][layer_index].bound, layer_index, ker.voxelDim, ker.queue, ker.pointID, ker.standardFactor);
				StripPlaning(clayerss[partnum][layer_index].bound, scanparameter.Part[partnum].Fill.间隙,
					scanparameter.Gobal.光斑半径补偿, rotate, AirOutlet::Right, para.Strip.条带宽度, para.Strip.延伸长度, fill, contour,para.Contour.偏置次数);
				VariPathss vp = GeneratePaths(fill, vf, ker.voxelDim, scanparameter.Part[partnum].Kernel.scanLenth
					, scanparameter.Part[partnum].Kernel.power_high, scanparameter.Part[partnum].Kernel.power_low);
				VariableSacn(vp, scanparameter.Part[partnum].Kernel.速度, 10000);

				if (contour.size() != 0 && scanparameter.Part[partnum].Contour.IsContour == true) {
					std::sort(contour[0].begin(), contour[0].end(), [](const Clipper2Lib::Path64& a, const Clipper2Lib::Path64& b) {
						return a[0].x > b[0].x; });

					for (int i = 0; i < contour.size(); i++) {
						VariPathss vpcb = GeneratePathsContour(contour[i], vf, ker.voxelDim, scanparameter.Part[partnum].Kernel.scanLenth
							, scanparameter.Part[partnum].Contour.功率, scanparameter.Part[partnum].Contour.功率低);
						VariableSacn(vpcb, scanparameter.Part[partnum].Contour.速度, 10000);
						ShowVariPathss(vpcb, para.Contour.功率, para.Contour.功率低);
					}
					if (IsStimulate && fill.size() != 0 && contour.size() != 0) {
						GetMarkJumpTime(fill, contour[0], para.Fill.速度, para.Contour.速度, marktime, jumptime);
						if (layer_Gobal_index % 50 == 0) {
							//ShowVariPathss(vpcb, scanparameter.Part[partnum].Kernel.power_high, scanparameter.Part[partnum].Kernel.power_low);
							//ShowVariPathss(vpcb, scanparameter.Part[partnum].Kernel.power_high, scanparameter.Part[partnum].Kernel.power_low);
						}
					}
				}
			}
			else if (scanparameter.Part[partnum].scanmode == ScanMode::线状支撑) {
				CBoundary cb = clayerss[partnum][layer_index].bound;
				Clipper2Lib::Paths64 solution = CBoundaryFloatToInt64(cb);
				if (solution.size() != 0) {
					std::sort(solution.begin(), solution.end(), [](const Clipper2Lib::Path64& a, const Clipper2Lib::Path64& b) {
						return a[0].x > b[0].x; });
					scanLineSupport(solution, scanparameter.Part[partnum].Contour.功率, scanparameter.Part[partnum].Contour.速度);
				}
			}

		}


		ScanSingle(120, 1000);
		if (layer_Gobal_index == ZLayersMax)
			break;
		auto endTime1 = std::chrono::steady_clock::now();
		//自适应铺粉技术
		float baseArea = Clipper2Lib::PI * 67.5 * 67.5;
		double addThickness = 0;
		if (layer_Gobal_index > 40)
			AutoFeed(baseArea, entityArea, scanparameter.Gobal.打印层厚, addThickness);
		else
			addThickness = scanparameter.Gobal.打印层厚;
		//运动
		if (!IsStimulate) {
			Z_F_move(-0.5 - scanparameter.Gobal.打印层厚, -0.5);
			Axis_Move(C, -310);
			Z_F_move(0.5, 0.5 + 2 * scanparameter.Gobal.打印层厚 + addThickness );
			Axis_Move(C, 310);
		}

		if (IsStimulate) {
			outputFile2 << layer_Gobal_index << "," << marktime << "," << jumptime << std::endl;
			timeTotal += marktime;
			timeTotal += jumptime;
			timeTotal += 7.300;
		}
		auto endTime3 = std::chrono::steady_clock::now();

		auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(endTime1 - startTime1);
		auto duration3 = std::chrono::duration_cast<std::chrono::milliseconds>(endTime3 - endTime1);
		outputFile << layer_Gobal_index << "," << duration1.count() << "," << duration3.count() << std::endl;
		std::cout << "wholeLayers" << ZLayersMax << ". " << "layer: " << layer_Gobal_index << " has finished" << std::endl;
	}
	
	scanFree();
	if (IsStimulate) {
		std::cout << "Total Precess Time :" << timeTotal / 3600 << std::endl;
		std::cout << "Total Precess Time :" << timeTotal / 3600 << std::endl;
		std::cout << "Total Precess Time :" << timeTotal / 3600 << std::endl;
	}
	outputFile.close();
	outputFile2.close();
	//全部置为零
	ProcessFlag.store(false, std::memory_order_release);
	Rs422 rs;
	out_data.MW10.laser_on = 0;
	rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
	ModbusTaskPush(rs);

	out_data.MW10.lase_power = 0;
	rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
	ModbusTaskPush(rs);

	out_data.MW10.laser_indicator = 0;
	rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
	ModbusTaskPush(rs);

	out_data.MW10.laser_enabled = 0;
	rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
	ModbusTaskPush(rs);

	//Axis_Move(C, 0);
	std::cout << "Finish Process!!!!" << std::endl;
}
template <typename T>
void PathMaker(ScanMode scanMode, T& data);


//void AxisFeed()

void ScanCircles(int nums_x, int nums_y, float interval_x, float interval_y, float radius, float power, float speed) {
	const int m_step_ratio = 5882;
	unsigned int busy, position;
	do
	{
		get_status(&busy, &position);
	} while (busy);

	int energy = 7.536 * power + 304.41;
	double speedset = speed / 1000 * m_step_ratio;
	write_da_1(energy);
	set_start_list(1);
	set_mark_speed(speedset);


	const float begin_x = -nums_x / 2 * interval_x;
	const float begin_y = -nums_y / 2 * interval_y;

	//横线
	n_jump_abs(1, -interval_x *m_step_ratio, 0);
	n_mark_abs(1, 0, 0);
	//竖线
	n_jump_abs(1, 0, -interval_y * m_step_ratio);
	n_mark_abs(1, 0, 0);

	


	for (int i = 0; i < nums_y; i++) {
		for (int j = 0; j < nums_x; j++) {
			int cur_x = begin_x + j * interval_x;
			int cur_y = begin_y + i * interval_y;
			n_jump_abs(1, (cur_x - radius) * m_step_ratio, cur_y * m_step_ratio);
			n_arc_abs(1, cur_x * m_step_ratio, cur_y *m_step_ratio, 360);
		}
	}

	set_end_of_list();
	execute_list(1U);
	do
	{
		get_status(&busy, &position);
	} while (busy);
}

void ScanLiness(int nums_x, int nums_y, float interval_x, float interval_y, float radius, float power, float speed) {
	const int m_step_ratio = 5882;
	unsigned int busy, position;
	do
	{
		get_status(&busy, &position);
	} while (busy);

	int energy = 7.536 * power + 304.41;
	double speedset = speed / 1000 * m_step_ratio;
	write_da_1(energy);
	set_start_list(1);
	set_mark_speed(speedset);


	const float begin_x = -nums_x / 2 * interval_x;
	const float begin_y = -nums_y / 2 * interval_y;

	//圆心
	n_jump_abs(1, ( - radius) * m_step_ratio, (0) * m_step_ratio);
	n_arc_abs(1, (0) * m_step_ratio, (0) * m_step_ratio, 270);


	//先扫横线
	for (int i = 0; i < nums_y; i++) {
		int cur_x = begin_x;
		int cur_y = begin_y + i * interval_y;
		n_jump_abs(1, cur_x * m_step_ratio, cur_y * m_step_ratio);
		n_mark_abs(1, -cur_x * m_step_ratio, cur_y * m_step_ratio);
	}
	//再扫竖线
	for (int j = 0; j < nums_x; j++) {
		int cur_x = begin_x + j * interval_x;
		int cur_y = begin_y ;
		n_jump_abs(1, cur_x * m_step_ratio, cur_y * m_step_ratio);
		n_mark_abs(1, cur_x * m_step_ratio, -cur_y * m_step_ratio);
	}
	set_end_of_list();
	execute_list(1U);
	do
	{
		get_status(&busy, &position);
	} while (busy);
}
void ScanCrossLines(int nums_x, int nums_y, float interval_x, float interval_y, float length, float power, float speed) {
	const int m_step_ratio = 5882;
	unsigned int busy, position;
	do
	{
		get_status(&busy, &position);
	} while (busy);

	int energy = 7.536 * power + 304.41;
	double speedset = speed / 1000 * m_step_ratio;
	write_da_1(energy);
	set_start_list(1);
	set_mark_speed(speedset);


	const float begin_x = -nums_x / 2 * interval_x;
	const float begin_y = -nums_y / 2 * interval_y;

	//圆心
	n_jump_abs(1, (-length) * m_step_ratio, (0) * m_step_ratio);
	n_arc_abs(1, (0) * m_step_ratio, (0) * m_step_ratio, 270);


	//先扫横线
	for (int i = 0; i < nums_y; i++) {
		for (int j = 0; j < nums_x; j++) {
			int cur_x = begin_x + j * interval_x;
			int cur_y = begin_y+ i*interval_y;
			n_jump_abs(1, (cur_x-length) * m_step_ratio, cur_y * m_step_ratio);
			n_mark_abs(1, (cur_x+length) * m_step_ratio, cur_y * m_step_ratio);

			n_jump_abs(1, (cur_x ) * m_step_ratio, (cur_y-length) * m_step_ratio);
			n_mark_abs(1, (cur_x ) * m_step_ratio, (cur_y+length )* m_step_ratio);
		}
	}

	set_end_of_list();
	execute_list(1U);
	do
	{
		get_status(&busy, &position);
	} while (busy);
}

void ZT_ARC(float c_x, float c_y, float tolerance, float radius, int ratio) {
	double therta = 2 * acos(1 - tolerance / radius);
	int arc_points = 2 * Clipper2Lib::PI / therta;
	therta = 2 * Clipper2Lib::PI / arc_points;

	for (int i = 0; i < arc_points; i++) {
		int cur_x = (c_x + radius * cos(i * therta))*ratio;
		int cur_y = (c_y + radius * sin(i * therta))*ratio;

		if (i == 0) {
			jump_abs(cur_x, cur_y);
		}
		else {
			mark_abs(cur_x, cur_y);
		}
	}
	mark_abs((c_x + radius) * ratio, (c_y)*ratio);

}
void MarkCircles(int nums_x, int nums_y, float interval_x, float interval_y, float radius, float tolerance,float power, float speed,bool delay) {
	

	unsigned int busy, position;
	do
	{
		get_status(&busy, &position);
	} while (busy);



	const int m_step_ratio = 5882;
	int energy = 7.536 * power + 304.41;
	double speedset = speed / 1000 * m_step_ratio;
	
	write_da_1(energy);
	set_start_list(1);
	if (delay) {
		set_delay_mode_list(1, 0, 50 / 10, 10, 588);
	}
		set_mark_speed(speedset);
		
		
		//横线
		n_jump_abs(1, -interval_x * m_step_ratio, 0);
		n_mark_abs(1, 0, 0);
		//竖线
		n_jump_abs(1, 0, -interval_y * m_step_ratio);
		n_mark_abs(1, 0, 0);



		const float begin_x = -nums_x / 2 * interval_x;
		const float begin_y = -nums_y / 2 * interval_y;


		for (int i = 0; i < nums_y; i++) {
			for (int j = 0; j < nums_x; j++) {
				int cur_x = begin_x + j * interval_x;
				int cur_y = begin_y + i * interval_y;
				ZT_ARC(cur_x, cur_y, tolerance, radius, m_step_ratio);
			}
		}

		set_end_of_list();
		execute_list(1U);
		do
		{
			get_status(&busy, &position);
		} while (busy);

}


void Bitmap(int resolution_x,int resolution_y,double dot_size, float power,float PixelPeriod ) {
	unsigned int busy, position;
	do
	{
		get_status(&busy, &position);
	} while (busy);

	const int m_step_ratio = 5882;
	int DotSize = dot_size * m_step_ratio;
	set_start_list(1);
		for (int i = 0; i < resolution_y; i++) {
			int start_x = -resolution_x * DotSize;
			int start_y = -resolution_y * DotSize / 2 + i * DotSize;
			jump_abs(start_x, start_y);
			set_pixel_line(1, PixelPeriod*32, DotSize, 0);
			double cur_power = 50+(power - 50) / resolution_y * i;
			int energy = 7.536 * cur_power + 304.41;
			set_n_pixel(1275U, energy, resolution_x);
		}
		
		for (int i = 0; i < resolution_y; i++) {
			int start_x = 0;
			int start_y = -resolution_y * DotSize / 2 + i * DotSize;
			jump_abs(start_x, start_y);
			set_pixel_line(1, PixelPeriod * 32, DotSize, 0);
			double cur_power = power-(power - 50) / resolution_y * i;
			int energy = 7.536 * cur_power + 304.41;
			set_n_pixel(1275U, energy, resolution_x);
		}

	set_end_of_list();
	execute_list(1U);
	do
	{
		get_status(&busy, &position);
	} while (busy);
}

ScanLines BitmapScanLines(int resolution_x, int resolution_y, double dot_size, float power, float PixelPeriod) {

	ScanLines ret;

	const int m_step_ratio = 5882;
	int DotSize = dot_size * m_step_ratio;
	const float speed = 1000;
	for (int i = 0; i < resolution_y; i++) {
		int start_x = -resolution_x * DotSize;
		int start_y = -resolution_y * DotSize / 2 + i * DotSize;
		
		int end_x = -resolution_x/10 * DotSize;

		double cur_power = 50 + (power - 50) / resolution_y * i;
		Clipper2Lib::Point64 p1({ start_x,-start_y });
		Clipper2Lib::Point64 p2({ end_x, -start_y });
		Clipper2Lib::Path64 path = { p1,p2 };
		ScanLine line(path, cur_power, speed);
		ret.push_back(line);
	}

	for (int i = 0; i < resolution_y; i++) {
		int start_x = 0;
		int start_y = -resolution_y * DotSize / 2 + i * DotSize;
		
		int end_x = resolution_x * DotSize;

		double cur_power = power - (power - 50) / resolution_y * i;
		
		Clipper2Lib::Point64 p1({ start_x,-start_y });
		Clipper2Lib::Point64 p2({ end_x, -start_y });
		Clipper2Lib::Path64 path = { p1,p2 };
		ScanLine line(path, cur_power, speed);
		ret.push_back(line);
	}
	return ret;

}
void DynamicRectLines(double length, double interval_y,int nums_x,int nums_y, double power,double speed) {
	unsigned int busy, position;
	do
	{
		get_status(&busy, &position);
	} while (busy);


	const int m_step_ratio = 5882;
	double interval_x = length / nums_x;
	double speedset = speed / 1000 * m_step_ratio;
	set_start_list(1);
		for (int i = 0; i < nums_y; i++) {
			for (int j = 0; j < nums_x; j++) {
				double cur_power = 50 + (power - 50) / nums_x * j;
				int energy = 7.536 * cur_power + 304.41;
				write_da_1_list(energy);
				set_mark_speed(speedset);
				int start_x = ( - length / 2 + interval_x * j)* m_step_ratio;
				int start_y = (-nums_y / 2 * interval_y + i * interval_y)* m_step_ratio;
				int end_x = (-length / 2 + interval_x * (j+1)) * m_step_ratio;

				if (j == 0) {
					jump_abs(start_x, start_y);
				}
				else {
					mark_abs(start_x, start_y);
				}
			}
		}



	set_end_of_list();
	execute_list(1U);


	do
	{
		get_status(&busy, &position);
	} while (busy);
}
ScanLines DynamicRectLinesScanLines(double length, double interval_y, int nums_x, int nums_y, double power, double speed) {
	ScanLines ret;
	
	const int m_step_ratio = 5882;
	double interval_x = length / nums_x;
	double speedset = speed / 1000 * m_step_ratio;
	for (int i = 0; i < nums_y; i++) {
		for (int j = 0; j < nums_x; j++) {
			double cur_power = 50 + (power - 50) / nums_x * j;

			int start_x = (-length / 2 + interval_x * j) * m_step_ratio;
			int start_y = (-nums_y / 2 * interval_y + i * interval_y) * m_step_ratio;
			int end_x = (-length / 2 + interval_x * (j + 1)) * m_step_ratio;

			

			Clipper2Lib::Point64 p1({ start_x,start_y });
			Clipper2Lib::Point64 p2({ end_x, start_y });
			Clipper2Lib::Path64 path = { p1,p2 };
			ScanLine line(path, cur_power, speed);
			ret.push_back(line);
		}
	}
	return ret;
}
void ArchimedeanSpirals(double radius,double interval,int sampleTimes, float power,float speed) {
	unsigned int busy, position;
	do
	{
		get_status(&busy, &position);
	} while (busy);

	double pi = 3.1415926;
	const int m_step_ratio = 5882;
	double speedset = speed / 1000 * m_step_ratio;
	set_start_list(1);
		jump_abs(0, 0);
		set_mark_speed(speedset);
		for (int i = 0; i < sampleTimes; i++) {
			double cur_power = (power - 50) / sampleTimes * i;
			int energy = 7.536 * cur_power + 304.41;
			write_da_1_list(energy);

			double angle =  2 * pi * 3 / sampleTimes * i;
			double x = (radius + angle / pi / 2 * interval) * cos(angle);
			double y = (radius + angle / pi / 2 * interval) * sin(angle);

			mark_abs(x * m_step_ratio, y * m_step_ratio);
		}
	set_end_of_list();
	execute_list(1U);


	do
	{
		get_status(&busy, &position);
	} while (busy);
}
ScanLines ArchimedeanSpiralsScanLines(double radius, double interval, int sampleTimes, float power, float speed){
	double pi = 3.1415926;
	const int m_step_ratio = 5882;
	double speedset = speed / 1000 * m_step_ratio;
	ScanLines ret;
	for (int i = 0; i < sampleTimes; i++) {
		double cur_power = 50 + (power - 50) / sampleTimes  * i;


		double angle = 2 * pi * 3 / sampleTimes * i;
		double x = (radius + angle / pi / 2 * interval) * cos(angle);
		double y = (radius + angle / pi / 2 * interval) * sin(angle);

		double angle1 = 2 * pi * 3 / sampleTimes * (i+1);
		double x1 = (radius + angle1 / pi / 2 * interval) * cos(angle1);
		double y1 = (radius + angle1 / pi / 2 * interval) * sin(angle1);

		Clipper2Lib::Point64 p1({ x * m_step_ratio, -y * m_step_ratio });
		Clipper2Lib::Point64 p2({ x1 * m_step_ratio, -y1 * m_step_ratio });
		Clipper2Lib::Path64 path = { p1,p2 };
		ScanLine line(path, cur_power, speed);
		ret.push_back(line);
	}

	return ret;
}
void Love(double radius, int sampleTimes, float power, float speed) {
	unsigned int busy, position;
	do
	{
		get_status(&busy, &position);
	} while (busy);

	double pi = 3.1415926;
	const int m_step_ratio = 5882;
	double speedset = speed / 1000 * m_step_ratio;
	set_start_list(1);
	//jump_abs(0, 0);
	set_mark_speed(speedset);
		for (int i = 0; i < sampleTimes/2; i++) {
			double cur_power = 50+(power - 50) / sampleTimes*2 * i;
			int energy = 7.536 * cur_power + 304.41;
			write_da_1_list(energy);

			double angle = -pi/2+2 * pi  / sampleTimes * i;
			double row = radius * (1 - sin(angle));
			double x = row * cos(angle);
			double y = row * sin(angle);

			if (i == 0) {
				jump_abs(x * m_step_ratio, y * m_step_ratio);
			}
			else
				mark_abs(x * m_step_ratio, y * m_step_ratio);
		}

		for (int i = 0; i < sampleTimes / 2; i++) {
			double cur_power = 50+(power - 50) / sampleTimes*2 * i;
			int energy = 7.536 * cur_power + 304.41;
			write_da_1_list(energy);

			double angle = -pi/2 - 2 * pi / sampleTimes * i;
			double row = radius * (1 - sin(angle));
			double x = row * cos(angle);
			double y = row * sin(angle);
			if (i == 0) {
				jump_abs(x * m_step_ratio, y * m_step_ratio);
			}else
				mark_abs(x * m_step_ratio, y * m_step_ratio);
		}
	set_end_of_list();
	execute_list(1U);


	do
	{
		get_status(&busy, &position);
	} while (busy);
}


ScanLines LoveScanLines(double radius, int sampleTimes, float power, float speed) {
	double pi = 3.1415926;
	const int m_step_ratio = 5882;
	ScanLines ret;

	for (int i = 0; i < sampleTimes / 2; i++) {
		double cur_power = 50 + (power - 50) / sampleTimes * 2 * i;

		double angle = -pi / 2 + 2 * pi / sampleTimes * i;
		double row = radius * (1 - sin(angle));
		double x = row * cos(angle);
		double y = row * sin(angle);

		double angle1 = -pi / 2 + 2 * pi / sampleTimes * ((i+1));
		double row1 = radius * (1 - sin(angle1));
		double x1 = row1 * cos(angle1);
		double y1 = row1 * sin(angle1);

		Clipper2Lib::Point64 p1({ x * m_step_ratio, y * m_step_ratio });
		Clipper2Lib::Point64 p2({x1 * m_step_ratio, y1 * m_step_ratio});
		Clipper2Lib::Path64 path = { p1,p2 };
		ScanLine line(path, cur_power, speed);
		ret.push_back(line);
	}

	for (int i = 0; i < sampleTimes / 2; i++) {
		double cur_power = 50 + (power - 50) / sampleTimes * 2 * i;

		double angle = -pi / 2 - 2 * pi / sampleTimes * i;
		double row = radius * (1 - sin(angle));
		double x = row * cos(angle);
		double y = row * sin(angle);

		double angle1 = -pi / 2 -2 * pi / sampleTimes * ((i + 1));
		double row1 = radius * (1 - sin(angle1));
		double x1 = row1 * cos(angle1);
		double y1 = row1 * sin(angle1);

		Clipper2Lib::Point64 p1({ x * m_step_ratio, y * m_step_ratio });
		Clipper2Lib::Point64 p2({ x1 * m_step_ratio, y1 * m_step_ratio });
		Clipper2Lib::Path64 path = { p1,p2 };
		ScanLine line(path, cur_power, speed);
		ret.push_back(line);
	}

	return ret;
}


Clipper2Lib::PathsD Generate(double angle_delta, int counter) {
	double begin_angle = 135;
	double radius_long = 45;
	double radius_short = 10;


	Clipper2Lib::PathsD ret;
	for (int i = 0; i < counter; i++) {
		Clipper2Lib::PathD p;
		double cur_angle = begin_angle - i * angle_delta;
		p.push_back({ cos(cur_angle/PI_180) * radius_long,sin(cur_angle/PI_180) * radius_long });
		p.push_back({ cos(cur_angle/ PI_180) * radius_short,sin(cur_angle/ PI_180) * radius_short });
		ret.push_back(p);
	}
	

	return ret;
}


void ScanSingle(float power,float speed) {
	static int layer = -1;

	layer++;
	Clipper2Lib::PathsD pathd = Generate(12.5, 9);


	Clipper2Lib::Paths64 paths;

	if (layer >= pathd.size()) return;
	int i = layer;
	Clipper2Lib::Path64 p;
	for (int j = 0; j < pathd[i].size(); j++) {
		int64_t x = pathd[i][j].x * 5882;
		int64_t y = pathd[i][j].y * 5882;
		Clipper2Lib::Point64 p1(x, y);
		p.push_back(p1);
	}
	paths.push_back(p);
	



	scanFill(paths, power, speed);
}