#include"Rtc5Scan.h"
// Useful constants
const UINT      ResetCompletely(UINT_MAX);
const LONG      R(58820L);                  //  size parameter of the figures
const LONG      R1(59143L);
const LONG      R2(65114L);

// Change these values according to your system
const UINT      DefaultCard(1U);            //  number of default card
const UINT      LaserMode(1U);              //  YAG
const UINT      LaserControl(0x18U);        //  Laser signals LOW active
//  (Bit 3 and 4)
 
// RTC4 compatibility mode assumed
const UINT      StandbyHalfPeriod(100U * 8U); //  100 us [1/8 us]
const UINT      StandbyPulseWidth(1U * 8U);   //    0 us [1/8 us]
const UINT      LaserHalfPeriod(100U * 8U);   //  100 us [1/8 us]
const UINT      LaserPulseWidth(50U * 8U);    //   50 us [1/8 us]
const LONG      LaserOnDelay(100L * 1L);      //  100 us [1 us]
const UINT      LaserOffDelay(100U * 1U);     //  100 us [1 us]


const double    MarkSpeed(5882);           //  1000mm/s
const double    JumpSpeed(17646);          //  3000mm/s
const polygon square1[] =
{
	  {-R-100000, -R}
	, {-R-100000,  R}
	, {R-100000,  R}
	, {R-100000, -R}
};

const polygon square2[] =
{
	  {-R1-100000, -R1}
	, {-R1-100000,  R1}
	, {R1-100000,  R1}
	, {R1-100000, -R1}
	, {-R1-100000, -R1}
};
const polygon square3[] =
{
	  {-R2, -R2}
	, {-R2,  R2}
	, {R2,  R2}
	, {R2, -R2}
	, {-R2, -R2}
};



int scanInitial()
{
	 //Scan Calibration factor K [Bit/mm]

	UINT ErrorCode = init_rtc5_dll();
	UINT rtcCards = rtc5_count_cards();

	ErrorCode = select_rtc(rtcCards);
	stop_execution();  

	ErrorCode = load_correction_file("c:\\home\\YMBuild\\RTC5_pbam\\right2.ct5",   // initialize like "D2_1to1.ct5",
		1U,  // table; #1 is used by default
		2U);
	ErrorCode = load_program_file("c:\\home\\YMBuild\\RTC5_pbam");

	
	select_cor_table(1, 0);
	reset_error(-1);  //Clear all previous error codes
	config_list(UINT_MAX, //use the list space as a single list
		0U);
	set_laser_mode(1);	    //1 CO2 laser selected	
	set_firstpulse_killer(20 * 64U);	//3����CV  1bit equals 1/64us
	set_laser_control(0);//0x18 bit#4 and #5 low active;0 high active
	//set_standby(100 * 8,      // half of the standby period in 1/8 microseconds
	//	8);         // pulse width in 1/8 microseconds
	
	set_standby(640U,      // half of the standby period in 1/64 microseconds
		64U);         // pulse width in 1/64 microseconds
	
	set_start_list(1);
		//long_delay(100000);//1s
		 
		set_scanner_delays(20U, 15U, 5U);// 1 bit equals 10 us
		set_laser_delays(370L, 480U);//2024/5/16  1 bit equals 0.5us,wheras RTC4 1bit equals 1us
		set_laser_pulses(640U, 1275U);//�Ƿ���Ҫ�д���ʵ
		set_jump_speed(JumpSpeed);
		set_mark_speed(MarkSpeed);//1 bit per ms
	set_end_of_list();
	execute_list(1);
		//write_da_1(1510U);//160W m_energy = 7.536 * p_scanPara.laserPower + 304.41;  //ʵ��������ʽ
	
	// Finish

	return 0;
}
int scanInitial(const std::string&rtc_crt_file)
{
	//Scan Calibration factor K [Bit/mm]

	UINT ErrorCode = init_rtc5_dll();
	UINT rtcCards = rtc5_count_cards();

	ErrorCode = select_rtc(rtcCards);
	stop_execution();

	ErrorCode = load_correction_file(rtc_crt_file.c_str(),   // initialize like "D2_1to1.ct5",
		1U,  // table; #1 is used by default
		2U);
	ErrorCode = load_program_file("c:\\home\\YMBuild\\RTC5_pbam");


	select_cor_table(1, 0);
	reset_error(-1);  //Clear all previous error codes
	config_list(UINT_MAX, //use the list space as a single list
		0U);
	set_laser_mode(1);	    //1 CO2 laser selected	
	set_firstpulse_killer(20 * 64U);	//3����CV  1bit equals 1/64us
	set_laser_control(0);//0x18 bit#4 and #5 low active;0 high active
	//set_standby(100 * 8,      // half of the standby period in 1/8 microseconds
	//	8);         // pulse width in 1/8 microseconds

	set_standby(640U,      // half of the standby period in 1/64 microseconds
		64U);         // pulse width in 1/64 microseconds

	set_start_list(1);
	//long_delay(100000);//1s

	set_scanner_delays(20U, 15U, 5U);// 1 bit equals 10 us
	set_laser_delays(370L, 480U);//2024/5/16  1 bit equals 0.5us,wheras RTC4 1bit equals 1us
	set_laser_pulses(640U, 1275U);//�Ƿ���Ҫ�д���ʵ
	set_jump_speed(JumpSpeed);
	set_mark_speed(MarkSpeed);//1 bit per ms
	set_end_of_list();
	execute_list(1);
	//write_da_1(1510U);//160W m_energy = 7.536 * p_scanPara.laserPower + 304.41;  //ʵ��������ʽ

// Finish

	return 0;
}
int scanLines(int layers)
{
	unsigned int busy, position;
	do
	{
		get_status(&busy, &position);
	} while (busy);

	int begin = layers % 4;
	write_da_1(1284U);
	set_start_list(1);
	set_mark_speed(MarkSpeed);
	jump_abs(square1[begin].xval,square1[begin].yval);
	for (int i = begin+1; i < begin+5 ; i++) {
		int tmp = i % 4;
		mark_abs(square1[tmp].xval, square1[tmp].yval);
	}
	set_end_of_list();
	execute_list(1U);


	do
	{
		get_status(&busy, &position);
		Sleep(20);
	} while (busy);

	
	return 0;
}
int scanFree()
{
	write_da_1(0);
	home_position(-10, -10);
	printf("Scan Finished!\n");
	terminateDLL();
	return 0;
}

void draw(const polygon* figure, const size_t& size)
{
	if (size)
	{
		unsigned int busy, position;
		do
		{
			get_status(&busy, &position);
		} while (busy);
		write_da_1(1284U);
		  
		set_start_list(1);
		set_mark_speed(MarkSpeed);
		jump_abs(figure->xval, figure->yval);
		size_t i(0U);
		for (figure++; i < size - 1U; i++, figure++)
		{
			mark_abs(figure->xval, figure->yval);
		}

		set_end_of_list();
		execute_list(1U);
		do
		{
			get_status(&busy, &position);
			Sleep(20);
		} while (busy);
	}
}

//  terminateDLL
//
//  Description
//
//  The function waits for a keyboard hit
//  and then calls free_rtc5_dll().
//  

void terminateDLL(void)
{
	/*printf("- Press any key to exit.\n");*/

	//while (!_kbhit()) {}
	//printf("\n");

	free_rtc5_dll();
}

//void scanContour(Clipper2Lib::Paths64& paths64) {
//	unsigned int busy, position;
//	do
//	{
//		get_status(&busy, &position);
//	} while (busy);
//	write_da_1(754U);
//	set_start_list(1);
//
//	for (auto& path64 : paths64)
//	{
//		jump_abs(path64[0].x, path64[0].y);
//		for(int i = 1 ; i< path64.size();i++)
//		{
//			mark_abs(path64[i].x, path64[i].y);
//		}
//		mark_abs(path64[0].x, path64[0].y);
//	}
//	set_end_of_list();
//	execute_list(1U);
//}
void scanFill(Clipper2Lib::Paths64& paths64,float power,float speed) {
	unsigned int busy, position;
	do
	{
		get_status(&busy, &position);
	} while (busy);
	int energy = 7.536 * power + 304.41;
	double speedset = speed / 1000 * 5882;
	write_da_1(energy);
	set_start_list(1);
	set_mark_speed(speedset);
	for (auto& i : paths64) {
		
		jump_abs(i[0].x, i[0].y);
		mark_abs(i[1].x, i[1].y);
		
	}

	set_end_of_list();
	execute_list(1U);

	
	do
	{
		get_status(&busy, &position);
		Sleep(20);
	} while (busy); 
	
}
void scanLineSupport(Clipper2Lib::Paths64& paths64, float power, float speed)
{
	unsigned int busy, position;
	do
	{
		get_status(&busy, &position);
	} while (busy);
	int energy = 7.536 * power + 304.41;
	double speedset = speed / 1000 * 5882;
	write_da_1(energy);
	set_start_list(1);
	set_mark_speed(speedset);
	//  Busy & 0x0001: list is still running
//  Busy & 0x00fe: list has finished, but home_jump is still active
//  Not possible after stop_execution:
//  Busy & 0xff00: list paused (pause_list or set_wait)
	////while(!load_list(1U,0U)){}
	for (auto i : paths64) {
		jump_abs(i[0].x, i[0].y);
		for (int j = 1; j < i.size(); j++) {
			mark_abs(i[j].x, i[j].y);
		}
	}
	set_end_of_list();
	execute_list(1U);

	do
	{
		get_status(&busy, &position);
		Sleep(20);
	} while (busy);
}
void scanContour(Clipper2Lib::Paths64& paths64,float power,float speed) {
	unsigned int busy, position;
	do
	{
		get_status(&busy, &position);
	} while (busy);
	int energy = 7.536 * power + 304.41;
	double speedset = speed / 1000 * 5882;
	write_da_1(energy);
	set_start_list(1);
	set_mark_speed(speedset);
	//  Busy & 0x0001: list is still running
//  Busy & 0x00fe: list has finished, but home_jump is still active
//  Not possible after stop_execution:
//  Busy & 0xff00: list paused (pause_list or set_wait)
	////while(!load_list(1U,0U)){}
	for (auto i : paths64) {
		jump_abs(i[0].x, i[0].y);
		for (int j = 1; j < i.size(); j++) {
			mark_abs(i[j].x, i[j].y);
		}
		mark_abs(i[0].x, i[0].y);
	}
	set_end_of_list();
	execute_list(1U);

	do
	{
		get_status(&busy, &position);
		Sleep(20);
	} while (busy);
}
void VariableSacn(const VariPathss& path) {
	unsigned int busy, position;
	do
	{ 
		get_status(&busy, &position);
	} while (busy);
	set_start_list(1);
	for (auto& i : path) {
		jump_abs(i.JumpPoint.x, i.JumpPoint.y);
		for (auto& j : i.MarkPoint) {
			int energy = 7.536 * j.power + 304.41;
			//double speedset = j.speed / 1000 * 5882;
			write_da_1_list(energy);
			//set_mark_speed(speedset);
			mark_abs(j.point.x, j.point.y);
		}
	}

	set_end_of_list();
	execute_list(1U);

	do
	{
		get_status(&busy, &position);
		Sleep(20);
	} while (busy);
}
void VariableSacn(const VariPathss& path, double speed,const int commandNum) {
	unsigned int busy, position;
	do
	{
		get_status(&busy, &position);
	} while (busy);
	int counter = 0;

	set_start_list(1);
	double speedset = speed / 1000 * 5882;
	set_mark_speed(speedset);
	for (auto& i : path) {
		jump_abs(i.JumpPoint.x, i.JumpPoint.y);
		for (auto& j : i.MarkPoint) {
			if (counter > commandNum)
			{
				counter = 0;
					int energy = 7.536 * j.power + 304.41;
					//double speedset = j.speed / 1000 * 5882;
					write_da_1_list(energy);
					//set_mark_speed(speedset);
					mark_abs(j.point.x, j.point.y);
				set_end_of_list();
				execute_list(1U);

				do
				{
					get_status(&busy, &position);
				} while (busy);
				set_start_list(1);

			}
			int energy = 7.536 * j.power + 304.41;
			//double speedset = j.speed / 1000 * 5882;
			write_da_1_list(energy);
			//set_mark_speed(speedset);
			mark_abs(j.point.x, j.point.y);
			counter += 2;
		}
	}

	set_end_of_list();
	execute_list(1U);
	do
	{
		get_status(&busy, &position);
		Sleep(20);
	} while (busy);
}