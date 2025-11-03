#pragma once
#include<queue>
#include<modbus.h>

//MB		modbus
//CharmRay	创瑞
//
typedef unsigned __int16 MB_Reg;
typedef  __int16 MB_INT;	//经过确认为有符号数
typedef  __int32 MB_DINT;	//


//为了实现PLC的接口文档与程序的直接对应，这里使用了‘位域’
//位域的使用详见：https://www.cnblogs.com/Philip-Tell-Truth/p/5805242.html
#pragma pack(push, 2)
struct CharmRayPlcRegs
{
	//Modbus地址40011
	struct
	{
		unsigned short security : 1;		//0
		unsigned short light : 1;			//1
		unsigned short red_light : 1;		//2
		unsigned short greeen_light : 1;	//3
		unsigned short blue_light : 1;		//4
		unsigned short alarm : 1;			//5
		unsigned short big_valve : 1;		//6
		unsigned short small_valve : 1;		//7
		unsigned short ventilate : 1;		//8
		unsigned short lase_power : 1;		//9
		unsigned short fan_power : 1;		//10
		unsigned short motor_power : 1;		//11
		unsigned short laser_indicator : 1;	//12
		unsigned short laser_enabled : 1;	//13
		unsigned short laser_on : 1;		//14
		unsigned short fan_on : 1;			//15

	}MW10;

	//Modbus地址40012
	struct Move
	{
		unsigned short Z轴回原点指令 : 1;		//0
		unsigned short F轴回原点指令 : 1;		//1
		unsigned short C轴回原点指令 : 1;		//2
		unsigned short Z轴运动开始指令 : 1;		//3
		unsigned short F轴运动开始指令 : 1;		//4
		unsigned short C轴运动开始指令 : 1;		//5
		unsigned short unused_6_15 : 10;		//6-15, 共10位
	}MW11;

	//Modbus地址40013
	struct
	{
		//MW13
		unsigned short 大充气阀状态 : 1;		//0
		unsigned short 小充气阀状态 : 1;		//1
		unsigned short 排气阀状态 : 1;			//2
		unsigned short 急停安全继电器状态 : 1;	//3
		unsigned short 腔门插到位信号 : 1;		//4
		unsigned short 腔门锁上电状态 : 1;		//5
		unsigned short 激光器有电信号: 1;		//6
		unsigned short 风机有电信号: 1;			//7
		unsigned short 电机有电信号 : 1;		//8
		//MW14	
		unsigned short Z轴原点标志 : 1;			//9
		unsigned short F轴原点标志 : 1;			//10
		unsigned short C轴原点标志 : 1;			//11
		//
		unsigned short unused_12_15 : 4;		//第12位至第15位，共计4位
	}MW13_MW14; 

	//Modbus地址40014
	struct 
	{
		unsigned short 激光器报警 : 1;			//0
		unsigned short 风机报警 : 1;			//1
		unsigned short unused_2_15 : 14;		//第2位到第15位，共计14位
	}MW12;


	//Modbus地址40015
	struct
	{
		unsigned short Z轴驱动器报警: 1;		//0
		unsigned short Z轴运动超时报警 : 1;		//1
		unsigned short F轴驱动器报警 : 1;		//2
		unsigned short F轴运动超时报警 : 1;		//3
		unsigned short C轴驱动器报警 : 1;		//4
		unsigned short C轴运动超时报警 : 1;		//5
		unsigned short Z轴正限位 : 1;			//6
		unsigned short F轴正限位 : 1;			//7
		unsigned short C轴正限位 : 1;			//8
		unsigned short Z轴负限位 : 1;			//9
		unsigned short F轴负限位 : 1;			//10
		unsigned short C轴负限位 : 1;			//11
		unsigned short Z轴运动结束信号 : 1;		//12
		unsigned short F轴运动结束信号 : 1;		//13
		unsigned short C轴运动结束信号 : 1;			//14
		unsigned short 铺粉到位 : 1;				//15

	}MW12_12_14;
//	//todo 用模板or宏换成 MB_UnusedReg<13, 53> ux;

	//Modbus地址40016
	struct {
		unsigned short Z轴运动停止信号 : 1;		//0
		unsigned short F轴运动停止信号 : 1;		//1
		unsigned short C轴运动停止信号 : 1;		//2
		unsigned short unused_3_15 : 13;		//第3位到第15位，共计13位
	}STOP;
	//Mobus地址40017-40020
	MB_Reg unused_17_20[4];

	//Mobus地址40021-40038
	MB_DINT Z轴加速度;
	MB_DINT Z轴运动速度;
	MB_DINT Z轴移动距离;

	MB_DINT F轴加速度;
	MB_DINT F轴运动速度;
	MB_DINT F轴移动距离;

	MB_DINT C轴加速度;
	MB_DINT C轴运动速度;
	MB_DINT C轴移动距离;


	//Modbus地址40039-40040
	MB_INT unused_39_40[2];

	//Modbus地址40041-40043
	MB_INT 风压设定值;
	MB_INT 压力预警设定值;
	MB_INT 压力报警设定值;

	//Modbus地址40044-40050
	MB_Reg usused44_50[7];

	//Modbus地址40051-40054
	MB_INT 氧含量低精度;
	MB_INT 氧含量高精度;
	MB_INT 风压实际值;
	MB_INT 腔体压力;

	//Modbus地址40055-40060
	MB_Reg usused55_60[6];

	//Modbus地址40061-40066
	MB_DINT Z轴当前位置;
	MB_DINT F轴当前位置;
	MB_DINT C轴当前位置;

};
#pragma pack(pop)

//todo
#define setbit(dest,bit) dest |= (1<<bit) //将Dest的第bit位置1
#define clrbit(dest,bit) dest &=~(1<<bit) //将dest的第bit位清0

struct FunCode {
	int funCode;//1是灯光，2是电机供电，3加工
	bool value;//0 or 1 ; 或者脉冲数
	float dis;
};//等着被删除把

struct Rs422 {
	bool mode;//0为读，1为写
	int address;
	int number;
	uint16_t* dest = nullptr;//怎么防止内存泄露
	//Rs422(bool Mode, int Address, int Number, uint16_t* Dest) :mode(Mode), address(Address), number(Number), dest(Dest) {};
	void change(bool Mode, int Address, int Number, uint16_t* Dest) {
		mode = Mode;
		address = Address;
		number = Number;
		dest = Dest;
	}
}; 

static struct {
	char device[32] = "COM2";//串行端口名称
	int baud = 19200;//波特率
	char parity = 'N';//奇偶校验
	int data_bit = 8;//指定数据的位数
	int stop_bit = 1;//指定停止位位数
}rtu_set_;

enum Axis {
	Z = 0,
	F,
	C
};
const float Motor_ratio[3] = { 5000,5000,95.2 };  //Z , F , C

static modbus_t* rtu_var_;
//

static std::queue<FunCode>task;
static  CharmRayPlcRegs out_data;//管理控制开关
static CharmRayPlcRegs MyData;
void ModbusInitial();
void ModbusTaskPush(Rs422&);
void taskPush(FunCode&);
void ReadRegister(); 
void ShowUICOMMand();
void Z_F_move(float, float);
void Axis_Move(Axis,float);
void MoveCom();
void Modbus();
CharmRayPlcRegs& get();
CharmRayPlcRegs* write();