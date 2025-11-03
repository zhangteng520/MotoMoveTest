#include<chrono>
#include<mutex>
#include<shared_mutex>
#include<thread>
#include<condition_variable>
#include<fstream>
#include<iostream>


#include"control.h"
using namespace std;

//void Manufacturing(CLayers clayers, float &interval, double &rotate_angle,
//	float& Z_Delta);//功率和扫描速度在ScanInitial

mutex rs422_mtx;
mutex task_mtx;
mutex modbus_mtx;
shared_mutex mutex_;
condition_variable queueCondVar;
condition_variable ModbusQueueCondVar;
std::queue<Rs422>modbus_task;

void ModbusInitial() {
	//modbus连接
	rtu_var_ = modbus_new_rtu(rtu_set_.device, rtu_set_.baud, rtu_set_.parity, rtu_set_.data_bit, rtu_set_.stop_bit);


	//设置从站
	int r = modbus_set_slave(rtu_var_, 1);
	if (r == -1) { cout << "Modbus_set_slave error" << endl; }

	//是否连接
	r = modbus_connect(rtu_var_);
	if (r == -1) { cout << "Modbus_connect error" << endl; }

	//设置超时时间
	modbus_set_byte_timeout(rtu_var_, 0, 100000);
	modbus_set_response_timeout(rtu_var_, 0, 100000);

	unsigned int initial = 0;
	r = modbus_write_registers(rtu_var_, 10, 2, (uint16_t*)&initial);
	if (r == -1) { cout << "Initial error" << endl;; }

	float Z_Move_Distance = 0.5;
	float F_Move_Distance = 1;
	float C_Move_Distance = 20;


	static float Z_Accelerate = 12;
	static float Z_Velocity = 3;
	static float F_Accelerate = 12;
	static float F_Velocity = 3;
	static float C_Accelerate = 350;
	static float C_Velocity = 150;

	out_data.Z轴加速度 = Z_Accelerate * Motor_ratio[Z];
	out_data.Z轴运动速度 = Z_Velocity * Motor_ratio[Z];
	out_data.Z轴移动距离 = Z_Move_Distance * Motor_ratio[Z];
	out_data.F轴加速度 = F_Accelerate * Motor_ratio[F];
	out_data.F轴运动速度 = F_Velocity * Motor_ratio[F];
	out_data.F轴移动距离 = F_Move_Distance * Motor_ratio[F];
	out_data.C轴加速度 = C_Accelerate * Motor_ratio[C];
	out_data.C轴运动速度 = C_Velocity * Motor_ratio[C];
	out_data.C轴移动距离 = C_Move_Distance * Motor_ratio[C];
	modbus_write_registers(rtu_var_, 20, 18, (uint16_t*)&out_data.Z轴加速度);


}
void ModbusTaskPush(Rs422& rs422) {
	{
		lock_guard<mutex>lg2(modbus_mtx);
		modbus_task.push(rs422);
	}
	ModbusQueueCondVar.notify_one();
}
void taskPush(FunCode &funcode) {
	{
		lock_guard<std::mutex> lg1(task_mtx);
		task.push(funcode);
	}
	queueCondVar.notify_one();
}
CharmRayPlcRegs& get() {
	shared_lock<shared_mutex>lck(mutex_);
	return MyData;
}
CharmRayPlcRegs* write() {
	unique_lock<shared_mutex>lck(mutex_);
	return &MyData;
}



 
void ReadRegister() {
	Rs422 rs422;//rs422生命周期
	while (true) {
		this_thread::sleep_for(chrono::milliseconds(200));
		rs422.change(0,10, 56, (uint16_t*)write());//这里的write()锁的生命周期
		ModbusTaskPush(rs422);
	}
}




void Z_F_move(float Z_distance, float F_distance) {
	out_data.Z轴移动距离 = Motor_ratio[Z] * Z_distance;
	out_data.F轴移动距离 = Motor_ratio[F] * F_distance;
	Rs422 rs422;
	rs422.change(1, 24, 8, (uint16_t*)&out_data.Z轴移动距离);
	ModbusTaskPush(rs422);

	out_data.MW11.Z轴运动开始指令 = 1;
	out_data.MW11.F轴运动开始指令 = 1;
	rs422.change(1, 11, 1, (uint16_t*)&out_data.MW11);
	ModbusTaskPush(rs422);


	int count = 0; 
	while (count < 3000) {
		count++;
		if(count == 1)
			this_thread::sleep_for(chrono::milliseconds(500));
		else
			this_thread::sleep_for(chrono::milliseconds(10));
		if (get().MW12_12_14.F轴运动结束信号 == 1  ){
			out_data.MW11.Z轴运动开始指令 = 0;
			out_data.MW11.F轴运动开始指令 = 0;
			rs422.change(1,11, 1, (uint16_t*)&out_data.MW11);
			ModbusTaskPush(rs422);
			break;
		}
	}
	if (count == 2999) cout << "未到位" << endl;

}

void Axis_Move(Axis axis, float distance) {
	//用于传参
	Rs422 rs422;

	//回零
	if (abs(distance) < 1e-6) {
		*(uint16_t*)&out_data.MW11 |= (1 << axis);
	}
	//开始运动
	else
	{
		*(&out_data.Z轴移动距离 + axis * 3) = distance * Motor_ratio[axis];
		rs422.change(1, 24 + 6 * axis, 2, (uint16_t*)(&out_data.Z轴移动距离 + 3 * axis));
		ModbusTaskPush(rs422);


		*(uint16_t*)&out_data.MW11 |= (1 << (axis + 3));//3为bit值偏移量
	}

	rs422.change(1, 11, 1, (uint16_t*)&out_data.MW11);
	ModbusTaskPush(rs422);

	int count = 0;
	while (count < 6000) {
		count++;	//第一次sleep时间大一些
		if (count == 1)
			this_thread::sleep_for(chrono::milliseconds(500));
		else
			this_thread::sleep_for(chrono::milliseconds(10));//读到旧值很可怕
		bool moveStopFlag = (*(uint16_t*)(&get().MW12_12_14) & (1 << (12 + axis))) >> (12 + axis);
		//if (((*(uint16_t*)( &get().MW12_12_14)  &  (1 << (axis + 12)) )>> 12)== 1) {//这句话you'wen'ti
		if (moveStopFlag == 1) {
			//回零
			if (abs(distance) < 1e-6) {
				*(uint16_t*)&out_data.MW11 &= (~(1 << axis));
			}
			//开始运动
			else
			{
				*(uint16_t*)&out_data.MW11 &= (~(1 << (axis + 3)));//3为bit值偏移量
			}
			rs422.change(1, 11, 1, (uint16_t*)&out_data.MW11);
			ModbusTaskPush(rs422);
			break;
		}
	}
	if (count == 6000) cout << "未到位" << endl;



}
void ShowUICOMMand() {
	SetConsoleOutputCP(CP_UTF8);
	CharmRayPlcRegs temp;
	auto startTime = std::chrono::steady_clock::now();
	while (true) {
		this_thread::sleep_for(chrono::milliseconds(200));
		auto currentTime = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (currentTime - startTime);
		system("cls");
		temp = get();
		cout << "当前时间： " << duration.count() << " ms" << endl;
		cout << "LIGHT: " << temp.MW10.light << endl;
		cout << "MOTOR_POWER: " << temp.MW10.motor_power << endl;
		cout << "MOTOR_ON: " << temp.MW13_MW14.电机有电信号 << endl;
		cout << "轴：\t\t" << "Z轴\t" << "F轴\t" << "C轴\t" << endl;
		cout << "速度：\t\t" << temp.Z轴运动速度 << "\t" << temp.F轴运动速度 << "\t" << temp.C轴运动速度 << "\t" << endl;
		cout << "加速度：\t" << temp.Z轴加速度 << "\t" << temp.F轴加速度 << "\t" << temp.C轴加速度 << "\t" << endl;
		cout << "移动距离：\t" << temp.Z轴移动距离 << "\t" << temp.F轴移动距离 << "\t" << temp.C轴移动距离 << "\t" << endl;
		cout << "MoveBotton: \t" << temp.MW11.Z轴运动开始指令 << "\t" << temp.MW11.F轴运动开始指令 << "\t" << temp.MW11.C轴运动开始指令 << "\t" << endl;
		cout << "停止指令：\t" << temp.STOP.Z轴运动停止信号 << "\t" << temp.STOP.F轴运动停止信号 << "\t" << temp.STOP.C轴运动停止信号 << "\t" << endl;
		cout << "回原点指令：\t" << temp.MW11.Z轴回原点指令 << "\t" << temp.MW11.F轴回原点指令 << "\t" << temp.MW11.C轴回原点指令 << "\t" << endl;
		cout << "正限位：\t" << temp.MW12_12_14.Z轴正限位 << "\t" << temp.MW12_12_14.F轴正限位 << "\t" << temp.MW12_12_14.C轴正限位 << "\t" << endl;
		cout << "负限位：\t" << temp.MW12_12_14.Z轴负限位 << "\t" << temp.MW12_12_14.F轴负限位 << "\t" << temp.MW12_12_14.C轴负限位 << "\t" << endl;
		cout << "回原点标志：\t" << temp.MW13_MW14.Z轴原点标志 << "\t" << temp.MW13_MW14.F轴原点标志 << "\t" << temp.MW13_MW14.C轴原点标志 << "\t" << endl;
		cout << "MoveStop： \t" << temp.STOP.Z轴运动停止信号 << "\t" << temp.STOP.F轴运动停止信号 << "\t" << temp.STOP.C轴运动停止信号 << "\t" << endl;
		cout << "运动结束： \t" << temp.MW12_12_14.Z轴运动结束信号 << "\t" << temp.MW12_12_14.F轴运动结束信号 << "\t" << temp.MW12_12_14.C轴运动结束信号 << "\t" << endl;
		cout << "驱动器报警: \t" << temp.MW12_12_14.Z轴驱动器报警 << "\t" << temp.MW12_12_14.F轴驱动器报警 << "\t" << temp.MW12_12_14.C轴驱动器报警 << "\t" << endl;
		cout << "运动超时报警: \t" << temp.MW12_12_14.Z轴运动超时报警 << "\t" << temp.MW12_12_14.F轴运动超时报警 << "\t" << temp.MW12_12_14.C轴运动超时报警 << "\t" << endl << endl;
		cout << "Z轴当前位置(-964—549036): " << temp.Z轴当前位置 << endl;
		cout << "F轴当前位置(37—540037): " << temp.F轴当前位置 << endl;
		cout << "C轴当前位置(127-30115): " << temp.C轴当前位置 << endl;
	}

}


void MoveCom() {
	FunCode Fc;
	Rs422 rs422;
	while (true) {
		{
			unique_lock<mutex>u1(task_mtx);
			queueCondVar.wait(u1, [] { return !task.empty(); });
			Fc = task.front();
			task.pop();
		}
		switch (Fc.funCode) {
		
		case 3: {
			
			Axis_Move(C, 0);
			this_thread::sleep_for(chrono::milliseconds(1000));
			
			Axis_Move(C, 300);


			int layer = 400;
			ofstream outputFile("process_square.csv");
			if (!outputFile)
				cout << "Fail to open file. " << endl;
			outputFile << "Layers" << "," << "Move Time cost / ms" <<","<<"scan" << endl;
			while (layer > 0) {
				auto startTime1 = std::chrono::steady_clock::now();
				Z_F_move(-0.5, -0.5);
				Axis_Move(C , -302);
				Z_F_move(0.45, 0.6);
				Axis_Move(C , 302);

				auto endTime1 = std::chrono::steady_clock::now();

				
				auto endTime2 = std::chrono::steady_clock::now();

				auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(endTime1 - startTime1);
				auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(endTime2 - endTime1);
				outputFile << layer << "," << duration1.count() <<","<<duration2.count() << endl;
				layer--;
			}
			outputFile.close();
			break;
		}
		case 8: {
			//scanInitial();
			//scanLines(0);
			//scanFree();
			break;
		}
		case 12: {
			//scanInitial();
	/*		scanFill();
			scanContour();*/
			//scanFree();
			break;
		}
		case 9: {
			Axis_Move(Z,Fc.dis);
			break;
		}
		case 10: {
			Axis_Move(F,Fc.dis);
			break;
		}
		case 11: {
			Axis_Move(C,Fc.dis);
			break;
		}
		case 15: {
			
			break;
		}
		}
	}
}

void Modbus() {
	Rs422 rs;
	while (true) {
		{
			unique_lock<mutex>u2(modbus_mtx);
			ModbusQueueCondVar.wait(u2, [&] { return !modbus_task.empty(); });
			rs = modbus_task.front();
			modbus_task.pop();
		}
		switch (rs.mode) {
		case 0: {
			int ret = modbus_read_registers(rtu_var_,rs.address, rs.number, rs.dest);
			if (ret == -1) {
				this_thread::sleep_for(chrono::milliseconds(100));
				ret = modbus_read_registers(rtu_var_, rs.address, rs.number, rs.dest);
				if (ret == -1)cout << "modbus read error" << endl;
			}
			break;
		}
		case 1: {
			int ret = modbus_write_registers(rtu_var_, rs.address, rs.number, rs.dest);
			if (ret == -1) {
				this_thread::sleep_for(chrono::milliseconds(100));
				ret = modbus_write_registers(rtu_var_, rs.address, rs.number, rs.dest);
				if (ret == -1)
					cout << "modbus write error" << endl;
			}
			break;
		}

		}

	}


}

