#pragma once
#include<stdlib.h>
#include<stdio.h>
#include<time.h>
#include<chrono>
#include<fstream>
#include<iostream>
#include<thread>
#include <map>
#include <filesystem>
#include <implot.h>

#include<Windows.h>
#include<mmsystem.h>
#pragma comment(lib,"winmm.lib")
#include "mainUI.h"
#include "control.h"
#include "slcReader.h"
#include "file.h"
#include "clipper2.h"
#include "Rtc5Scan.h"
#include "Manufacturing.h"
#include "Voxelization.h"
#include "tool.h"
#include "game.h"
#include "Render.h"
#pragma execution_character_set("utf-8") 

using namespace Clipper2Lib;
static std::atomic<bool>ProcessFlag(false);
static std::string rtc_crt_file = "c:\\home\\YMBuild\\RTC5_pbam\\right2.ct5";

static Vector2DPlot plot;
static std::vector<std::vector< ImVec2>> counters;
static std::vector< ImVec2> fill;
const static std::vector<RGBA> colorTable = {
	{255,0,0,255}   //红色
	,{0,0,255,255}	//蓝色
	,{60,179,113,255} //绿色
	,{238,130,238,255}// 紫色
	,{255,165,0,255}	//黄色
	,{106,90,205,255}	//紫蓝色
	,{0,0,0,255}		//黑色
	,{60,60,60,255}   //灰色
};
void ShowVectorPlotDemo()
{
	plot.Begin("2D Vector Plot", ImVec2(1200,900 ));

	plot.Render();
		//for (int i = 0; i < counters.size(); i++) {
	//	plot.DrawPolygon(counters[i], RGBAToUnsigned(colorTable[i+1]), false, 2.0f);
	//}
	//plot.DrawPolygon(fill, RGBAToUnsigned(colorTable[0]), false, 1.0f);
	//plot.DrawLine(ImVec2(-100, -50), ImVec2(100, 50), IM_COL32(255, 0, 0, 255), 2.0f);
	//plot.DrawCircle(ImVec2(0, 0), 30, IM_COL32(0, 255, 0, 255));
	//plot.DrawPolygon({ ImVec2(-50, -50), ImVec2(50, -50), ImVec2(0, 50) }, IM_COL32(0, 128, 255, 255), false, 2.0f);
	//plot.DrawBezier(ImVec2(-100, -100), ImVec2(-50, 100), ImVec2(50, -100), ImVec2(100, 100),
	//	IM_COL32(255, 255, 0, 255), 2.0f);

	plot.End();
}
void OxygenControl(float oxygenRatio, int charmberPressure) {
	//氧气含量自动控制
	while (ProcessFlag.load(std::memory_order_acquire) == true) {
		CharmRayPlcRegs temp = get();
		int OxygenRatioSet = oxygenRatio * 100;
		int OxygenRatio = temp.氧含量高精度;
		if (temp.MW10.big_valve == 0) {
			if (OxygenRatio > (OxygenRatioSet * 0.8)) {
				out_data.MW10 = temp.MW10;
				out_data.MW10.big_valve = 1;
				Rs422 rs3;
				rs3.change(1, 10, 1, (uint16_t*)&out_data.MW10);
				ModbusTaskPush(rs3);
			}
		}
		else {
			if (OxygenRatio < (OxygenRatioSet * 0.6)) {
				out_data.MW10 = temp.MW10;
				out_data.MW10.big_valve = 0;
				Rs422 rs3;
				rs3.change(1, 10, 1, (uint16_t*)&out_data.MW10);
				ModbusTaskPush(rs3);
			}
		}
		//if (temp.MW10.ventilate == 0) {
		//	if (temp.腔体压力 > charmberPressure) {
		//		out_data.MW10 = temp.MW10;
		//		out_data.MW10.ventilate = 1;
		//		Rs422 rs3;
		//		rs3.change(1, 10, 1, (uint16_t*)&out_data.MW10);
		//		ModbusTaskPush(rs3);
		//	}
		//}
		//else {
		//	if (temp.腔体压力 < charmberPressure * 0.2) {
		//		out_data.MW10 = temp.MW10;
		//		out_data.MW10.ventilate = 0;
		//		Rs422 rs3;
		//		rs3.change(1, 10, 1, (uint16_t*)&out_data.MW10);
		//		ModbusTaskPush(rs3);
		//	}
		//}
		std::this_thread::sleep_for(std::chrono::microseconds(1000));
	}
}
using namespace Clipper2Lib;
struct Component {
	CLayers clayers;
	std::string fileName;
	int IsSupport = 0;//0无支撑，1片状支撑，2块状支撑
};



int nums_x = 5;
int nums_y = 5;
float interval_x = 30;
float interval_y = 30;

float radius = 15;
float tolerance = 0.05;
int SampleTimes = 1000;
float t_power = 50;
float t_speed = 1000;
bool delay = false;
std::atomic<bool>flag;
std::atomic<int>x_num_right;
std::atomic<float> interval_right;
std::atomic<float> delay12;
std::atomic<float> power12;
std::atomic<float> speed12;

std::atomic<int>transition_num;
std::atomic<float> interval_right2;
std::atomic<float> power122;
std::atomic<float> speed122;
std::atomic<bool> stop;

std::atomic<float> powerRight2;
std::atomic<float>speedRight2;
void GradualPaths64() {
	float ratio = 5882;
	float begin_x = -30;
	float begin_y1 = 10;
	float begin_y2 = 40;

	float beginy1 = -10;
	float beginy2 = -20;
	int x_num_left = 120;
	float intervaleft = 0.08;

	float delay = 200;


	Axis_Move(C, 0);
	scanInitial();
	Axis_Move(C, 301);
	int layer_index = 0;
	while (flag.load(std::memory_order_acquire)==true) {
		//扫描
		while (stop.load(std::memory_order_acquire)) {
			Sleep(50);
		}

		if (layer_index < 20) {
			unsigned int busy, position;
			do
			{
				get_status(&busy, &position);
			} while (busy);
			int power = 7.536 * 100 + 304.41;
			double speedset = 1000 / 1000 * 5882;
			write_da_1(power);

			set_start_list(1);
			set_jump_speed(speedset);

			for (float x = begin_x - 5; x < begin_x + 15; x += 0.08) {
				jump_abs(x * m_stepRatio, (begin_y1 + 5) * m_stepRatio);
				mark_abs(x * m_stepRatio, (begin_y2 - 5) * m_stepRatio);
			}

			set_end_of_list();
			execute_list(1U);

			do
			{
				get_status(&busy, &position);
				Sleep(20);
			} while (busy);

			set_start_list(1);
			for (float x = begin_x - 5; x < begin_x + 15; x += 0.08) {
				jump_abs(x * m_stepRatio, (beginy1 - 5) * m_stepRatio);
				mark_abs(x * m_stepRatio, (beginy2 + 5) * m_stepRatio);
			}

			set_end_of_list();
			execute_list(1U);

			do
			{
				get_status(&busy, &position);
				Sleep(20);
			} while (busy);
		}
		else {
			float intervl = interval_right.load(std::memory_order_acquire);
			begin_x += intervl * x_num_right;
			//第一个件
			unsigned int busy, position;

			do
			{
				get_status(&busy, &position);
			} while (busy);
			int energy = 7.536 * 100 + 304.41;
			double speedset = 1000 / 1000 * 5882;

			set_start_list(1);
			//
			set_mark_speed(speedset);
			write_da_1_list(energy);
			int i = 1;
			for (i = 1; i <= x_num_left; i++) {
				int x = (begin_x + i * intervaleft) * ratio;
				if (i % 2 == 0) {
					int y1 = begin_y1 * ratio;
					int y2 = begin_y2 * ratio;
					jump_abs(x, y1);
					mark_abs(x, y2);
				}
				else {
					int y1 = begin_y1 * ratio;
					int y2 = begin_y2 * ratio;
					jump_abs(x, y2);
					mark_abs(x, y1);
				}

			}
			i--;

			set_end_of_list();
			execute_list(1U);
			do
			{
				get_status(&busy, &position);
			} while (busy);
			//过渡
			set_start_list(1);
			if (layer_index % 2 == 0)
				energy = 7.536 * power122.load(std::memory_order_acquire) + 304.41;
			else
				energy = 304.41;
			speedset = speed122.load(std::memory_order_acquire) / 1000 * 5882;
			int num = transition_num.load();
			float interval_transition = interval_right2.load();
			set_mark_speed(speedset);
			write_da_1_list(energy);
			int k = 1;
			for (k = 1; k <= num; k++) {
				int x = (begin_x + i * intervaleft + k * interval_transition) * ratio;
				if (k % 2 == 0) {
					int y1 = begin_y1 * ratio;
					int y2 = begin_y2 * ratio;
					jump_abs(x, y1);
					mark_abs(x, y2);
				}
				else {
					int y1 = begin_y1 * ratio;
					int y2 = begin_y2 * ratio;
					jump_abs(x, y2);
					mark_abs(x, y1);
				}
			}
			k--;

			set_end_of_list();
			execute_list(1U);


			do
			{
				get_status(&busy, &position);
			} while (busy);

			//右边1
			energy = 7.536 * power12.load(std::memory_order_acquire) + 304.41;
			speedset = speed12.load(std::memory_order_acquire) / 1000 * 5882;

			set_start_list(1);
			int delayInt = delay12.load(std::memory_order_acquire) * 100;
			set_mark_speed(speedset);
			write_da_1_list(energy);
			int x = (begin_x + i * intervaleft + intervl + k * interval_transition) * ratio;
			if ((k + 1) % 2 == 0) {
				long_delay(delayInt);
				int y1 = begin_y1 * ratio;
				int y2 = begin_y2 * ratio;
				jump_abs(x, y1);
				mark_abs(x, y2);
			}
			else {
				long_delay(delayInt);
				int y1 = begin_y1 * ratio;
				int y2 = begin_y2 * ratio;
				jump_abs(x, y2);
				mark_abs(x, y1);
			}


			set_end_of_list();
			execute_list(1U);


			do
			{
				get_status(&busy, &position);
				Sleep(20);
			} while (busy);

			//右边2
			energy = 7.536 * powerRight2.load(std::memory_order_acquire) + 304.41;
			speedset = speedRight2.load(std::memory_order_acquire) / 1000 * 5882;

			set_start_list(1);
			delayInt = delay12.load(std::memory_order_acquire) * 100;
			set_mark_speed(speedset);
			write_da_1_list(energy);
			x = (begin_x + i * intervaleft + 2 * intervl + k * interval_transition) * ratio;
			if ((k + 2) % 2 == 0) {
				long_delay(delayInt);
				int y1 = begin_y1 * ratio;
				int y2 = begin_y2 * ratio;
				jump_abs(x, y1);
				mark_abs(x, y2);
			}
			else {
				long_delay(delayInt);
				int y1 = begin_y1 * ratio;
				int y2 = begin_y2 * ratio;
				jump_abs(x, y2);
				mark_abs(x, y1);
			}



			set_end_of_list();
			execute_list(1U);


			do
			{
				get_status(&busy, &position);
				Sleep(20);
			} while (busy);

			//第二个件
			do
			{
				get_status(&busy, &position);
			} while (busy);
			energy = 7.536 * 100 + 304.41;
			speedset = 1000 / 1000 * 5882;

			set_start_list(1);
			//zuobian
			set_mark_speed(speedset);
			write_da_1_list(energy);
			i = 1;
			for (i = 1; i <= x_num_left; i++) {
				int x = (begin_x + i * intervaleft) * ratio;
				if (i % 2 == 0) {
					int y1 = beginy1 * ratio;
					int y2 = beginy2 * ratio;
					jump_abs(x, y1);
					mark_abs(x, y2);
				}
				else {
					int y1 = beginy1 * ratio;
					int y2 = beginy2 * ratio;
					jump_abs(x, y2);
					mark_abs(x, y1);
				}

			}
			i--;

			set_end_of_list();
			execute_list(1U);
			do
			{
				get_status(&busy, &position);
			} while (busy);
			//过渡
			set_start_list(1);
			if (layer_index % 2 == 0)
				energy = 7.536 * power122.load(std::memory_order_acquire) + 304.41;
			else
				energy = 304.41;
			speedset = speed122.load(std::memory_order_acquire) / 1000 * 5882;
			num = transition_num.load();
			interval_transition = interval_right2.load();
			set_mark_speed(speedset);
			write_da_1_list(energy);
			k = 1;
			for (k = 1; k <= num; k++) {
				int x = (begin_x + i * intervaleft + k * interval_transition) * ratio;
				if (k % 2 == 0) {
					int y1 = beginy1 * ratio;
					int y2 = beginy2 * ratio;
					jump_abs(x, y1);
					mark_abs(x, y2);
				}
				else {
					int y1 = beginy1 * ratio;
					int y2 = beginy2 * ratio;
					jump_abs(x, y2);
					mark_abs(x, y1);
				}
			}
			k--;

			set_end_of_list();
			execute_list(1U);


			do
			{
				get_status(&busy, &position);
			} while (busy);

			//右边1
			energy = 7.536 * power12.load(std::memory_order_acquire) + 304.41;
			speedset = speed12.load(std::memory_order_acquire) / 1000 * 5882;

			set_start_list(1);
			delayInt = delay12.load(std::memory_order_acquire) * 100;
			set_mark_speed(speedset);
			write_da_1_list(energy);
			x = (begin_x + i * intervaleft + intervl + k * interval_transition) * ratio;
			if ((k + 1) % 2 == 0) {
				long_delay(delayInt);
				int y1 = beginy1 * ratio;
				int y2 = beginy2 * ratio;
				jump_abs(x, y1);
				mark_abs(x, y2);
			}
			else {
				long_delay(delayInt);
				int y1 = beginy1 * ratio;
				int y2 = beginy2 * ratio;
				jump_abs(x, y2);
				mark_abs(x, y1);
			}


			set_end_of_list();
			execute_list(1U);


			do
			{
				get_status(&busy, &position);
				Sleep(20);
			} while (busy);

			//右边2
			energy = 7.536 * powerRight2.load(std::memory_order_acquire) + 304.41;
			speedset = speedRight2.load(std::memory_order_acquire) / 1000 * 5882;

			set_start_list(1);
			delayInt = delay12.load(std::memory_order_acquire) * 100;
			set_mark_speed(speedset);
			write_da_1_list(energy);
			x = (begin_x + i * intervaleft + 2 * intervl + k * interval_transition) * ratio;
			if ((k + 2) % 2 == 0) {
				long_delay(delayInt);
				int y1 = beginy1 * ratio;
				int y2 = beginy2 * ratio;
				jump_abs(x, y1);
				mark_abs(x, y2);
			}
			else {
				long_delay(delayInt);
				int y1 = beginy1 * ratio;
				int y2 = beginy2 * ratio;
				jump_abs(x, y2);
				mark_abs(x, y1);
			}



			set_end_of_list();
			execute_list(1U);


			do
			{
				get_status(&busy, &position);
				Sleep(20);
			} while (busy);

		}
		////yundong
		Z_F_move(-0.5 - 0.04, -0.5);
		Axis_Move(C, -300);
		Z_F_move(0.5, 0.5 + 3 * 0.04);
		Axis_Move(C, 300);
		layer_index++;
		std::cout << "LayerIsProcessed: " << layer_index << std::endl;
	}
	scanFree();



}
int showUI()
{
	//initial game
	srand(static_cast<unsigned>(time(0)));
	InitGrid();
	SpawnTetromino();
	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"ZTAM", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	// Initialize OpenGL
	if (!CreateDeviceWGL(hwnd, &g_MainWindow))
	{
		CleanupDeviceWGL(hwnd, &g_MainWindow);
		::DestroyWindow(hwnd);
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}
	wglMakeCurrent(g_MainWindow.hDC, g_hRC);

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.Fonts->Clear();
	io.Fonts->AddFontFromFileTTF("Font/dengb.ttf", 18.f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_InitForOpenGL(hwnd);
	ImGui_ImplOpenGL3_Init();

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// Our state
	bool show_demo_window = true;
	bool show_para_window = false;
	bool show_maohua_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	float pb_dis_ = 0.f;
	float feed_dis_ = 0.f;
	float rom_dis_ = 0.f;
	int layers = 175;
	int number = 50;
	// Main loop
	bool done = false;

	//Data
	SLCReader slcReader;
	std::vector <Component> Components;
	ScanParamter scanparameter;
	static int numbers = 0 ;

	//参考Manufacturing.h
	double thickness = 0.04;
	double ratioCompensation = 0.1;
	double oxygenratio = 0.1;
	double ChamberPressure = 3000;

	std::vector<double> fillPower;
	std::vector<double> fillSpeed;
	std::vector<double> contourPower;
	std::vector<double>contourSpeed;

	//激光毛化
	int NUMY = 20;
	double interval = 0.1;
	double X_Lenth = 100;
	double scan_x = 0.1;
	double begin_x = -250000;
	double begin_y = -250000;
	double power = 200;
	double speed = 1000;
	int time_delay = 1;
	//渐进路径
	int x_right_num1 = 2;
	double interval1 = 0.04;
	double power1 = 100;
	double speed1 = 1000;
	double delay1 = 100;

	int x_right_num2 = 2;
	double interval2 = 0.04;
	double power2 = 100;
	double speed2 = 1000;

	double speed3 = 1000;
	double power3 = 100;
	
	int   bar_data[3] = {100,10,100};
	float x_data[6] = {100,12,23,1,32,1};
	float y_data[6] = {100,1,52,5,15,12};


	while (!done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;


		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		UpdateGame();
		//game
		// 
		// 
		// 创建 ImGui 窗口
		ImGui::Begin("ImPlot Example");

		// 创建一个简单的折线图
		static float x_data[100];
		static float y_data[100];
		for (int i = 0; i < 100; ++i) {
			x_data[i] = i * 0.1f;
			y_data[i] = sin(x_data[i]);
		}

		if (ImPlot::BeginPlot("Example Plot")) {
			ImPlot::PlotLine("Sine Wave", x_data, y_data, 100);
			ImPlot::EndPlot();
		}

		ImGui::End();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			static int current = 0;
			static int counter = 0;

			ShowVectorPlotDemo();
			ImGui::Begin("主控台", NULL, 0);                          // Create a window called "Hello, world!" and append into it.            // Display some text (you can use a format strings too)

			//ImGui::Begin("Numeric Keypad", open);
			//static const char* keys = "7894561230";
			//for (int i = 0; i < 11; i++)
			//{
			//	if (i == 9)
			//		ImGui::Button("0");
			//	else
			//		ImGui::Button(&keys[i]);

			//	if ((i + 1) % 3 != 0)
			//		ImGui::SameLine();
			//}

			//ImGui::End();


			/*if (ImGui::BeginMenuBar()) {*/
			if (ImGui::BeginMenu("开始")) {
				
				if (ImGui::MenuItem("文件导入")) {
					//char filename[256];
					//SelectOpenFiles(filename);

					//Component componet;
					//std::filesystem::path filePath = filename;
					//componet.fileName = filePath.filename().string();
					//if (IsSupport(componet.fileName))
					//	componet.IsSupport = 1;
					//slcReader.clear();
					//slcReader.init(filename);
					//CLayers clayers = slcReader.layers();
					//componet.clayers = clayers;
					//Components.push_back(componet);
					std::vector<std::string> filenames;
					SelectOpenFiles(filenames);
					for (auto& filename : filenames)
					{
						Component componet;
						std::filesystem::path filePath = filename;
						componet.fileName = filePath.filename().string();
						if (IsSupport(componet.fileName))
						componet.IsSupport = 1;	
						slcReader.clear();
						slcReader.init(filename);
						CLayers clayers = slcReader.layers();
 						componet.clayers = clayers;
						Components.push_back(componet);
					}
					//逆风排序
					if(Components.size()!= 0)
						std::sort(Components.begin(), Components.end(), [](const Component& a, const Component& b)
						{ return a.clayers[0].bound[0][0].x > b.clayers[0].bound[0][0].x; });
				}
				if (ImGui::MenuItem("文件清除")) {
					Components.clear();
					scanparameter.Part.clear();

				}
				ImGui::EndMenu();
			}
			ImGui::Checkbox("参数配置", &show_para_window);
			if (show_para_window)
			{
				ImGui::Begin("参数配置", &show_para_window);
				numbers = Components.size();
				scanparameter.Part.resize(numbers);
				ImGui::Text("加载零件数 %d",numbers);

				ImGui::Text("全局变量");
				ImGui::InputDouble("打印层厚", &thickness,0.01f ,0.0,"%.2f");
				ImGui::InputDouble("光斑半径补偿", &ratioCompensation, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("氧气含量上限", &oxygenratio, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("腔体压力", &ChamberPressure, 1000.0f, 0.0, "%.1f");
				ImGui::Text("    ");
				
				if (numbers > 0) {
					ImGui::SliderInt("当前零件", &current, 0U, numbers - 1);
					ImGui::SetNextWindowSize(ImVec2(400, 100));
					ImGui::Text("文件名：%s", gb2312_to_utf8(Components[current].fileName).c_str()); //todo中文乱码
					ImGui::Text("当前文件层数：%d", Components[current].clayers.size()-1);
					
					if (Components[current].IsSupport == 0) {
						ImGui::SliderInt("填充模式", (int*)&scanparameter.Part[current].scanmode, 0U, 6);
						ImGui::Text("填充模式：%s", gb2312_to_utf8(enumToString(scanparameter.Part[current].scanmode)).c_str());

						if (scanparameter.Part[current].scanmode == ScanMode::Zigzag ||
							scanparameter.Part[current].scanmode == ScanMode::光栅扫描 ||
							scanparameter.Part[current].scanmode == ScanMode::最短空行程||
							scanparameter.Part[current].scanmode == ScanMode::条带路径) {
							ImGui::Text("    ");
							ImGui::Text("填充参数");
							ImGui::PushItemWidth(200.0f);
							ImGui::InputDouble("间隙", &scanparameter.Part[current].Fill.间隙, 0.0f, 0.0, "%.2f");
							ImGui::SameLine();
							ImGui::InputDouble("旋转角度°", &scanparameter.Part[current].Fill.旋转角度, 0.0f, 0.0, "%.2f");
							ImGui::InputDouble("功率", &scanparameter.Part[current].Fill.功率, 5.0f, 0.0, "%.1f");
							ImGui::SameLine();
							ImGui::InputDouble("速度", &scanparameter.Part[current].Fill.速度, 50.0f, 0.0, "%.1f");
							if (scanparameter.Part[current].scanmode == ScanMode::条带路径) {
								ImGui::InputDouble("条带宽度", &scanparameter.Part[current].Strip.条带宽度, 1.0f, 0.0, "%.1f");
								ImGui::SameLine();
								ImGui::InputDouble("延伸长度", &scanparameter.Part[current].Strip.延伸长度, 0.1f, 0.0, "%.2f");
							}

							float EnergyDensity = scanparameter.Part[current].Fill.功率 / (scanparameter.Part[current].Fill.间隙 * scanparameter.Part[current].Fill.速度 * scanparameter.Gobal.打印层厚);
							ImGui::Text("体能量密度: %.2f  (J/mm^3)", EnergyDensity);
							ImGui::Checkbox("轮廓参数", &scanparameter.Part[current].Contour.IsContour);
							if (scanparameter.Part[current].Contour.IsContour) {
								ImGui::InputDouble("功率##cC", &scanparameter.Part[current].Contour.功率, 0.0f, 0.0, "%.1f");
								ImGui::SameLine();
								ImGui::InputDouble("功率低##ccc", &scanparameter.Part[current].Contour.功率低, 0.0f, 0.0, "%.1f");
								ImGui::InputDouble("速度##cC", &scanparameter.Part[current].Contour.速度, 0.0f, 0.0, "%.1f");
								ImGui::SameLine();
								ImGui::InputInt("偏置次数##up", &scanparameter.Part[current].Contour.偏置次数);
							}

							ImGui::Checkbox("Upskin参数", &scanparameter.Part[current].Upskin.IsUpSkin);
							if (scanparameter.Part[current].Upskin.IsUpSkin == true) {
								ImGui::InputDouble("间隙##up", &scanparameter.Part[current].Upskin.间隙, 0.0f, 0.0, "%.2f");
								ImGui::SameLine();
								ImGui::InputDouble("旋转角度(相较于填充)##up°", &scanparameter.Part[current].Upskin.旋转角度, 0.0f, 0.0, "%.2f");
								ImGui::InputDouble("功率##up", &scanparameter.Part[current].Upskin.功率, 5.0f, 0.0, "%.1f");
								ImGui::SameLine();
								ImGui::InputDouble("速度##up", &scanparameter.Part[current].Upskin.速度, 50.0f, 0.0, "%.1f");
								ImGui::InputInt("上表面层号##up", &scanparameter.Part[current].Upskin.上表面层号);
								ImGui::SameLine();
								ImGui::InputInt("向下层数##up", &scanparameter.Part[current].Upskin.层数);

								ImGui::Checkbox("重熔", &scanparameter.Part[current].Upskin.IsMelt);
								if (scanparameter.Part[current].Upskin.IsMelt) {
									ImGui::InputDouble("重熔间隙##up", &scanparameter.Part[current].Upskin.重熔间隙, 0.0f, 0.0, "%.2f");
									ImGui::SameLine();
									ImGui::InputDouble("重熔旋转角度##up°", &scanparameter.Part[current].Upskin.重熔旋转角度, 0.0f, 0.0, "%.2f");
									ImGui::InputDouble("重熔功率##up", &scanparameter.Part[current].Upskin.重熔功率, 5.0f, 0.0, "%.1f");
									ImGui::SameLine();
									ImGui::InputDouble("重熔速度##up", &scanparameter.Part[current].Upskin.重熔速度, 50.0f, 0.0, "%.1f");
									ImGui::InputInt("重熔次数##up", &scanparameter.Part[current].Upskin.重熔次数);
								}
							}
							ImGui::PopItemWidth();
						}
						if (scanparameter.Part[current].scanmode == ScanMode::卷积核) {
							ImGui::Text("    ");
							ImGui::Text("卷积核参数");
							ImGui::PushItemWidth(200.0f);
							ImGui::InputDouble("间隙##c", &scanparameter.Part[current].Kernel.间隙, 0.0f, 0.0, "%.2f");
							ImGui::SameLine();
							ImGui::InputDouble("旋转角度°##c", &scanparameter.Part[current].Kernel.旋转角度, 0.0f, 0.0, "%.2f");
							ImGui::InputDouble("功率高##c", &scanparameter.Part[current].Kernel.power_high, 5.0f, 0.0, "%.1f");
							ImGui::InputDouble("功率低##c", &scanparameter.Part[current].Kernel.power_low, 5.0f, 0.0, "%.1f");
							ImGui::SameLine();
							ImGui::InputDouble("速度##c", &scanparameter.Part[current].Kernel.速度, 50.0f, 0.0, "%.1f");
							ImGui::InputDouble("体素精度##c", &scanparameter.Part[current].Kernel.precision, 50.0f, 0.0, "%.1f");
							ImGui::InputInt("卷积核半径##c", &scanparameter.Part[current].Kernel.num_xy);
							ImGui::SameLine();
							ImGui::InputInt("卷积核层数##c", &scanparameter.Part[current].Kernel.num_z);
							ImGui::InputDouble("扫描矢量长度##c", &scanparameter.Part[current].Kernel.scanLenth, 0.0f, 0.0, " % .2f");
							
							ImGui::Checkbox("轮廓参数", &scanparameter.Part[current].Contour.IsContour);
							if (scanparameter.Part[current].Contour.IsContour) {
								ImGui::InputDouble("功率高##d", &scanparameter.Part[current].Contour.功率, 0.0f, 0.0, "%.1f");
								ImGui::SameLine();
								ImGui::InputDouble("功率低##d", &scanparameter.Part[current].Contour.功率低, 0.0f, 0.0, "%.1f");
								ImGui::InputDouble("速度##d", &scanparameter.Part[current].Contour.速度, 0.0f, 0.0, "%.1f");
							}
							ImGui::PopItemWidth();
						}
					}
					if (Components[current].IsSupport != 0) {
						ImGui::SliderInt("填充模式(1:线状；2:块状)", &Components[current].IsSupport, 1U, 2);
						if (Components[current].IsSupport == 2) {
							scanparameter.Part[current].scanmode = ScanMode::块状支撑;
							ImGui::Text("填充参数");
							ImGui::PushItemWidth(200.0f);
							ImGui::InputDouble("间隙", &scanparameter.Part[current].Fill.间隙, 0.0f, 0.0, "%.2f");
							ImGui::SameLine();
							ImGui::InputDouble("旋转角度°", &scanparameter.Part[current].Fill.旋转角度, 0.0f, 0.0, "%.2f");
							ImGui::InputDouble("功率", &scanparameter.Part[current].Fill.功率, 5.0f, 0.0, "%.1f");
							ImGui::SameLine();
							ImGui::InputDouble("速度", &scanparameter.Part[current].Fill.速度, 50.0f, 0.0, "%.1f");
							ImGui::PopItemWidth();
						}
						if (Components[current].IsSupport == 1) {
							scanparameter.Part[current].scanmode = ScanMode::线状支撑;
							ImGui::PushItemWidth(200.0f);
							ImGui::Text("轮廓参数");
							ImGui::InputDouble("功率##c", &scanparameter.Part[current].Contour.功率, 0.0f, 0.0, "%.1f");
							ImGui::SameLine();
							ImGui::InputDouble("速度##c", &scanparameter.Part[current].Contour.速度, 0.0f, 0.0, "%.1f");

							ImGui::PopItemWidth();
						}
					}
				}
				ImGui::Text("    ");
				if (ImGui::Button("更新全局参数", ImVec2(150, 50))) {
					scanparameter.Gobal.光斑半径补偿 = ratioCompensation;
					scanparameter.Gobal.打印层厚 = thickness;
					scanparameter.Gobal.氧含量上限 = oxygenratio;
					scanparameter.Gobal.腔体压力上限 = ChamberPressure;

				}
				if (ImGui::Button("关闭小窗", ImVec2(100, 50)))
					show_para_window = false;
				ImGui::End();
			}
			ImGui::InputInt("层数", &layers);
			ImGui::InputInt("扫描线条数", &number);
			if (ImGui::Button("路径规划##cc")) {

				char filename[256];
				SelectOpenFiles(filename);
				slcReader.clear();
				slcReader.init(filename);
				CLayers clayers = slcReader.layers();


				int layer_index = layers;

					Clipper2Lib::Paths64 fill, contour;
					std::vector<Clipper2Lib::Paths64> contour1;
					int a = 67;
					double rotate = (a * layer_index + 0) % 360;
					StripPlaning(clayers[layer_index].bound, 0.08,
						0.1, rotate, AirOutlet::Right, 20, 0.1, fill, contour1, 1);

					if (number < fill.size())
						fill.resize(number);

					SvgWriter svg;//改色修改pencolor
					for (int i = 0; i < contour1.size(); i++) {
						svg.AddPaths(contour1[i], false, FillRule::Negative, 0x00000000, 0xFFFF0000, 1.3, false);
					}

					//svg.AddPaths(contour, false, FillRule::Negative, 0x00000000, 0xFFFF0000, 1.3, false);
					FillRule fr = FillRule::Negative;
					SvgAddOpenSubject(svg, fill, fr, false);
					SvgSaveToFile(svg,std::to_string(layer_index) + ".svg", 900, 1800, 20);
					System(std::to_string(layer_index) + ".svg");
					Sleep(1000);

			}
			
			if (ImGui::Button("路径规划##dd"))                        // Buttons return true when clicked (most widgets return true when edited/activated)
			{
				char filename[256];
				SelectOpenFiles(filename);
				slcReader.clear();
				slcReader.init(filename);
				CLayers clayers = slcReader.layers();
				
				Clipper2Lib::Paths64 fill, contour;
				int a = 67;
				double rotate = (a * layers + 0) % 360;
				Paths64Planing(clayers[layers].bound, 0.08,
					0.1, rotate, AirOutlet::Right, fill, contour);

				if (number < fill.size())
					fill.resize(number);

				SvgWriter svg;//改色修改pencolor
					svg.AddPaths(contour, false, FillRule::Negative, 0x00000000, 0xFFFF0000, 1.3, false);
				

				//svg.AddPaths(contour, false, FillRule::Negative, 0x00000000, 0xFFFF0000, 1.3, false);
				FillRule fr = FillRule::Negative;
				SvgAddOpenSubject(svg, fill, fr, false);
				SvgSaveToFile(svg,  "1.svg", 900, 1800, 20);
				System("1.svg");

		
			}

			
			ImGui::SameLine();

			if (ImGui::Button("毕业路径")) {

				plot.ClearScreen();

				char filename[256];
				SelectOpenFiles(filename);
				slcReader.clear();
				slcReader.init(filename);
				CLayers clayers = slcReader.layers();

				auto& one_layer = clayers[0].bound;

				//SvgWriter svg;//改色修改pencolor

				Clipper2Lib::Paths64 p = CBoundaryFloatToInt64(one_layer);
				p = SimplifyPaths(p, 0, true);
				//svg.AddPaths(p, false, FillRule::Negative, 0x00000000, 0xFF000000, 1.3, false);

				plot.DrawPaths64(ConnectPloygon(p), RGBAToUnsigned(colorTable[0]));
				//Clipper2Lib::JoinType jt = Clipper2Lib::JoinType::Round;
				//const float compensation = 0.1f;
				//int ratioCompensation = compensation * 5882;
				
				//p = InflatePaths(p, -ratioCompensation, jt, Clipper2Lib::EndType::Polygon);
				//p = SimplifyPaths(p, 50, false);
				//plot.DrawPaths64(ConnectPloygon(p), RGBAToUnsigned(colorTable[1]));
				//svg.AddPaths(p, false, FillRule::Negative, 0x00000000, 0xFFFF0000, 1.3, false);
				Edges edges = Paths64ToEdges(p, true);
				Clipper2Lib::Paths64 result;
				int intervaluse = 0.04 * 5882;
				EdgesToRaster(edges, intervaluse, true, true, result);

				LineCompensation(result, 0.1 * 5882);
				plot.DrawPaths64(result, RGBAToUnsigned(colorTable[2]));
				//svg.AddPaths(p, false, FillRule::Negative, 0x00000000, 0x00FF0000, 1.3, false);
				//fill = Paths64ToImVec2(result);
				//FillRule fr = FillRule::Negative;
				//SvgAddOpenSubject(svg, result, fr, false);
				//SvgSaveToFile(svg, "1.svg", 900, 1800, 20);
				//System("1.svg");



			}
			ImGui::Checkbox("激光毛化", &show_maohua_window);
			if (show_maohua_window)
			{
				ImGui::Begin("激光毛化",&show_maohua_window);
				ImGui::InputInt("打印Y层数", &NUMY, 1.0f, 0.0);
				ImGui::InputDouble("打印Y间隔", &interval, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("打印X长度", &X_Lenth, 1.0f, 0.0, "%.2f");
				ImGui::InputDouble("打印X间距", &scan_x, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("起始点X", &begin_x, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("起始点Y", &begin_y, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("功率", &power, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("速度", &speed, 0.01f, 0.0, "%.2f");
				ImGui::InputInt("时间延时", &time_delay);
				if (ImGui::Button("毛化")) {

					scanInitial();
					unsigned int busy, position;
					do
					{
						get_status(&busy, &position);
					} while (busy);

					int pow = power * 7.536 + 304.41;
					write_da_1(pow);
					set_start_list(1);
					set_mark_speed(speed / 1000 * 5882);
					int beginX = begin_x;
					int beginY = begin_y;
					for (int i = 0; i < NUMY; i++) {
						double Y = beginY + (i * interval * 5882.f);
						jump_abs(beginX, Y);

						for (double j = 0; j * scan_x < X_Lenth; j++) {
							//auto start = std::chrono::high_resolution_clock::now();
							srand(time(NULL));
							////auto end= std::chrono::high_resolution_clock::now();
							////auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
							////std::cout << duration.count() << std::endl;
							int x = rand();
							double ratio = ((double)x) / static_cast<double>(RAND_MAX);
							double X1 = beginX + j * scan_x * 5882 + 5882 * scan_x *ratio;
							double X2 = beginX + (j + 1) * scan_x * 5882;
							mark_abs(X1, Y);
							jump_abs(X2, Y);
							
						}
					}


					set_end_of_list();
					execute_list(1U);

					do
					{
						get_status(&busy, &position);
						Sleep(20);
					} while (busy);

					scanFree();


					//while (!load_list(1U, 0U)) {} //  wait for list 1 to be not busy
					////  load_list( 1, 0 ) returns 1 if successful, otherwise 0
					////  set_start_list_pos( 1, 0 ) has been executed

					//jump_abs(figure->xval, figure->yval);

					//size_t i(0U);
					//for (figure++; i < size - 1U; i++, figure++)
					//{
					//	mark_abs(figure->xval, figure->yval);
					//}

					//set_end_of_list();
					//execute_list(1U);
				}
				if (ImGui::Button("清洗")) {

					scanInitial();

					unsigned int busy, position;
					do
					{
						get_status(&busy, &position);
					} while (busy);
					int pow = power * 7.536 + 304.41;
					write_da_1(pow);
					 
					set_start_list(1);
					set_mark_speed(speed / 1000 * 5882);
					set_jump_speed(speed / 1000 * 5882);
					save_and_restart_timer();
					int beginX = begin_x;
					int beginY = begin_y;
					for (int i = 0; i < NUMY; i++) {
						double Y = beginY + (i * interval * 5882.f);
						jump_abs(beginX, Y);
						for (int j = 0; j * scan_x < X_Lenth; j++) {
							write_da_1(pow + 0.01 * j);
							double X1 = beginX + (j +1)* scan_x * 5882;
							mark_abs(X1, Y);
						}

					}
					double x = get_time();
					set_end_of_list();
					execute_list(1U);
					
					

					auto start = std::chrono::high_resolution_clock::now(); 
					do
					{
						get_status(&busy, &position);
						
					} while (busy);
					
					auto end = std::chrono::high_resolution_clock::now();
					auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
					std::cout << "scan time  " << x << std::endl;
					scanFree();
				}

				if (ImGui::Button("变功率")) {

					scanInitial();

					unsigned int busy, position;
					do
					{
						get_status(&busy, &position);
					} while (busy);
					int pow = power * 7.536 + 304.41;
					write_da_1(pow);

					set_start_list(1);
					set_mark_speed(speed / 1000 * 5882);
					set_jump_speed(speed / 1000 * 5882);   
					int beginX = begin_x;
					int beginY = begin_y;
					for (int i = 0; i < NUMY; i++) {
						double Y = beginY + (i * interval * 5882.f);
						jump_abs(beginX, Y);
						double X1 = beginX + X_Lenth * 5882;
						mark_abs(X1, Y);
					}
					set_end_of_list();
					execute_list(1U);
					timeBeginPeriod(1);
					for (int i = 0; i < 1000; i++) {
						
						// auto start = std::chrono::high_resolution_clock::now();
						//std::this_thread::sleep_for(std::chrono::microseconds(1000));
						::Sleep(time_delay);
	/*					auto end= std::chrono::high_resolution_clock::now();
							auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
							std::cout << duration.count() << std::endl;*/
						if (i % 2 == 0) {
							write_da_1(0);
						}
						else {
							write_da_1(pow);
						}
						
					}
					timeEndPeriod(1);
					auto start = std::chrono::high_resolution_clock::now();
					do
					{
						get_status(&busy, &position);
					} while (busy);

					auto end = std::chrono::high_resolution_clock::now();
					auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
					std::cout << "scan time  " << duration1.count() << std::endl;
					scanFree();
				}

				if (ImGui::Button("哈哈")) {
					scanInitial();
					unsigned int busy, position;
					do
					{
						get_status(&busy, &position);
					} while (busy);
					unsigned int x = get_list_space();
					std::cout << "empty: " << x << std::endl;
					set_start_list_1();
					jump_abs(10, 10);

					x = get_list_space();
					std::cout << "one: " << x << std::endl;

					mark_abs(100, 100);

					x = get_list_space();
					std::cout << "two: " << x << std::endl;
					
					set_end_of_list();
					 x = get_list_space();
					std::cout << "end: " << x << std::endl;
					execute_list(1U);

					x = get_list_space();
					std::cout << "execute " << x << std::endl;
					scanFree();
				}
				if (ImGui::Button("one")) {
					scanInitial();
					unsigned int busy, position;
					do
					{
						get_status(&busy, &position);
					} while (busy);
					
					set_start_list_1();

					save_and_restart_timer();
					jump_abs(-5*5882, 0);


					mark_abs(5*5882, 0);

					
					double x = get_time();
					set_end_of_list();
					
					execute_list(1U);


					

					do
					{
						get_status(&busy, &position);
					} while (busy);

					

					std::cout << "time " << x << std::endl; 
					scanFree();
				}
				ImGui::End();
			}
			if (1)
			{
				ImGui::Begin("GradualPath");


				ImGui::InputInt("过渡区域条数##c", &x_right_num2, 1.0f, 0.0);
				ImGui::InputDouble("过渡间隔##c", &interval2, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("功率##c", &power2, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("速度##c", &speed2, 0.01f, 0.0, "%.2f");
				ImGui::Separator();
				ImGui::InputInt("悬垂区域条数", &x_right_num1, 1.0f, 0.0);
				ImGui::InputDouble("右侧间隔", &interval1, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("延时ms", &delay1, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("功率1", &power1, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("速度1", &speed1, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("功率2", &power3, 0.01f, 0.0, "%.2f");
				ImGui::InputDouble("速度2", &speed3, 0.01f, 0.0, "%.2f");

				double angle = atan(0.04 / (interval1 * x_right_num1));
				int angle0 = angle * 180 / PI;
				ImGui::Text("角度%d°", angle0);
				if (ImGui::Button("更新加工参数 ")) {
					interval_right.store(interval1, std::memory_order_release);
					power12.store(power1, std::memory_order_release);
					speed12.store(speed1, std::memory_order_release);
					delay12.store(delay1, std::memory_order_release);
					x_num_right.store(x_right_num1, std::memory_order_release);

					interval_right2.store(interval2, std::memory_order_release);
					power122.store(power2, std::memory_order_release);
					speed122.store(speed2, std::memory_order_release);

					powerRight2.store(power3, std::memory_order_release);
					speedRight2.store(speed3, std::memory_order_release);
					transition_num.store(x_right_num2, std::memory_order_release);
				}
				if (ImGui::Button("开始加工 ")) {
					flag.store(true, std::memory_order_release);
					stop.store(false, std::memory_order_release);
					std::thread th2(GradualPaths64);
					th2.detach();
				}
				if (ImGui::Button("暂停")) {
					stop.store(true, std::memory_order_release);
				}
				if (ImGui::Button("继续")) {
					stop.store(false, std::memory_order_release);
				}
				if (ImGui::Button("结束加工 ")) {
					flag.store(false, std::memory_order_release);
				}
				ImGui::End();
			}

			if (ImGui::Button("Stimualtion")) {

				if (Components.size() != 0 && scanparameter.Part.size() != 0) {

					ProcessFlag.store(true);
					std::vector<CLayers> clayerss;
					for (auto& i : Components) {
						clayerss.push_back(i.clayers);
					}
					ProcessFlag.store(true, std::memory_order_release);
					std::thread th2(MultiPartsPrint, scanparameter, (clayerss), true,std::ref(ProcessFlag));
					th2.detach();
				}
			}
			if (ImGui::Button("process")) {
				
				if (Components.size() != 0&&scanparameter.Part.size()!= 0) {

					ProcessFlag.store(true);
					std::vector<CLayers> clayerss;
					for (auto& i : Components) {
						clayerss.push_back(i.clayers);
					}
					ProcessFlag.store(true, std::memory_order_release);
					std::thread th2(MultiPartsPrint, scanparameter,(clayerss),false,std::ref(ProcessFlag));
					th2.detach();
					std::thread th3(OxygenControl, scanparameter.Gobal.氧含量上限,(int)scanparameter.Gobal.腔体压力上限);
					th3.detach();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("processStop")) {
				ProcessFlag.store(false);
			}
			//if (ImGui::Button("Time")) {
			//	char filename[256];
			//	SelectOpenFiles(filename);
			//	slcReader.clear();
			//	slcReader.init(filename);
			//	CLayers clayers = slcReader.layers();
			//	std::ofstream outputFile("MarkAndJump3.csv");
			//	if (!outputFile)
			//		std::cout << "Fail to open file. " << std::endl;
			//	outputFile << "Layers" << "," << "MarkTime" << "," << "JumpTime" <<  std::endl;
			//	
			//	outputFile.close();
			//}

			
			if (1) {
				ImGui::Begin("能场调控", &show_maohua_window);
				ImGui::InputInt("层数", &layers);
				if (ImGui::Button("体素")) {
					char filename[256];
					SelectOpenFiles(filename);
					slcReader.clear();
					slcReader.init(filename);
					CLayers clayers = slcReader.layers();
					CubeD cube = GetCubf(clayers);
					VoxelDimension voxelDim;
					MakeVoxelDimension(cube, voxelDim);
					std::vector<std::vector<Point2ID>> pointID = makeSquereID(voxelDim.num_margin,12, voxelDim.precision,0.1);
					FixedSizeQueue<Voxel> queue(12);
					int standardFactor = StandardFactor(pointID);
					Voxel voxel = ExecVoxelization(clayers[layers].bound, voxelDim);
					for (auto& i : voxel) {
						for (auto& j : i) {
							if (j == 1)
								std::cout << "*";
							else
								std::cout << "0";
						}
						std::cout << std::endl;
					}
				}
				if (ImGui::Button("能量影响因子")) {
					char filename[256];
					SelectOpenFiles(filename);
					slcReader.clear();
					slcReader.init(filename);
					CLayers clayers = slcReader.layers();

					CubeD cube = GetCubf(clayers);
					VoxelDimension voxelDim;
					MakeVoxelDimension(cube,voxelDim);
					std::vector<std::vector<Point2ID>> pointID = makeSquereID(voxelDim.num_margin, 12,0.04,voxelDim.precision);
					FixedSizeQueue<Voxel> queue(12);
					for (int i = 0; i < 12; i++) {
						Voxel data(voxelDim.n_y, std::vector<unsigned char>(voxelDim.n_x, 1));
						queue.push(data);
					}
					int standardFactor = StandardFactor(pointID);
					for (int layer_index = 0; layer_index < clayers.size(); layer_index++) {
						VoxelFactor vf = CaculateFactor(clayers[layer_index].bound, layer_index, voxelDim, queue, pointID, standardFactor);
						if (layer_index >=number) {
							for (auto& i : vf) {
								for (auto& j : i) {
									if (abs(j - 1.0) > 1e-5)
										std::cout << std::fixed << std::setprecision(1) << j << " ";
									else
										std::cout << "  . ";
								}
								std::cout << std::endl;
							}
							break;
						}
					}
				}

				ImGui::End();
			}
			if (1) {
				ImGui::Begin("标定");
				ImGui::TextUnformatted(rtc_crt_file.c_str());
				ImGui::SameLine();
				if (ImGui::SmallButton("更改")) {
					//scanFree();
					char filename[256];
					SelectOpenFiles(filename);
					rtc_crt_file = gb2312_to_utf8(filename);

				}
				ImGui::InputInt("nums_x", &nums_x, 2);
				ImGui::InputInt("nums_y", &nums_y, 2);
				ImGui::InputFloat("interval_x", &interval_x, 5);
				ImGui::InputFloat("interval_y", &interval_y, 5);
				ImGui::InputFloat("radius", &radius, 1);
				ImGui::InputFloat("power", &t_power, 1);
				ImGui::InputFloat("speed", &t_speed, 1);
				ImGui::InputFloat("tolerance", &tolerance, 0.01);
				ImGui::InputInt("SampleTimes", &SampleTimes, 0.01);
				ImGui::Checkbox("delay", &delay);
				if (ImGui::Button("圆圈")) {
					scanInitial(utf8_to_gb2312(rtc_crt_file));
					//scanInitial();
					ScanCircles(nums_x, nums_y, interval_x, interval_y, radius, t_power, t_speed);
					scanFree();
				}

				if (ImGui::Button("直线")) {
					scanInitial(utf8_to_gb2312(rtc_crt_file));
					ScanLines(nums_x, nums_y, interval_x, interval_y, radius, t_power, t_speed);
					scanFree();
				}
				if (ImGui::Button("十字线")) {
					scanInitial(utf8_to_gb2312(rtc_crt_file));
					ScanCrossLines(nums_x, nums_y, interval_x, interval_y, radius, t_power, t_speed);
					scanFree();
				}
				if (ImGui::Button("多边形拟合")) {
					double therta = 2 * acos(1 - tolerance / radius);
					int arc_points = 2 * Clipper2Lib::PI / therta;
					std::cout << arc_points << std::endl;
					scanInitial(utf8_to_gb2312(rtc_crt_file));

					MarkCircles(nums_x, nums_y, interval_x, interval_y, radius, tolerance, t_power, t_speed,delay);
					scanFree();
				}
				if (ImGui::Button("BitMap")) {
					scanInitial(utf8_to_gb2312(rtc_crt_file));
					Bitmap(nums_x, nums_y, radius, t_power,SampleTimes);
					scanFree();
				}
				ImGui::SameLine();
				if (ImGui::Button("Love")) {
					scanInitial(utf8_to_gb2312(rtc_crt_file));
					Love(radius, SampleTimes, t_power, t_speed);
					scanFree();
				}
				ImGui::SameLine();
				if (ImGui::Button("Raster")) {
					scanInitial(utf8_to_gb2312(rtc_crt_file));
					DynamicRectLines(interval_x,interval_y,nums_x,nums_y, t_power, t_speed);
					scanFree();
				}
				ImGui::SameLine();
				if (ImGui::Button("Spiral")) {
					scanInitial(utf8_to_gb2312(rtc_crt_file));
					ArchimedeanSpirals(radius,interval_x,SampleTimes, t_power, t_speed);
					scanFree();
				}
				ImGui::End();
			}
			//ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

			//ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			//ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color


			//if (ImGui::Button("Button1"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			//	counter++;
			//ImGui::SameLine();
			//ImGui::Text("counter = %d", counter);

			CharmRayPlcRegs temp4 = get();//不会把&删除了就不会阻塞了吧
			FunCode func;
			Rs422 rs;
			//ImGui::Text(u8"张腾");
			if (ImGui::Button("Light", ImVec2(150, 50))) {
				out_data.MW10.light = ~temp4.MW10.light;
				rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
				ModbusTaskPush(rs);
			}

			if (ImGui::Button("Motor_power")) {
				out_data.MW10.motor_power = ~temp4.MW10.motor_power;
				rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
				ModbusTaskPush(rs);
			}
			ImGui::SameLine();
			ImGui::Text("statue: %d", temp4.MW13_MW14.电机有电信号);


			if (ImGui::Button("激光启动")) {
				out_data.MW10.laser_on = ~temp4.MW10.laser_on;
				rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
				ModbusTaskPush(rs);
			}

			ImGui::SameLine();
			if (ImGui::Button("激光供电")) {
				out_data.MW10.lase_power = ~temp4.MW10.lase_power;
				rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
				ModbusTaskPush(rs);
			}
			ImGui::SameLine();
			if (ImGui::Button("指示光")) {
				out_data.MW10.laser_indicator = ~temp4.MW10.laser_indicator;
				rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
				ModbusTaskPush(rs);
			}
			ImGui::SameLine();
			if (ImGui::Button("激光使能")) {
				out_data.MW10.laser_enabled = ~temp4.MW10.laser_enabled;
				rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
				ModbusTaskPush(rs);
			}

			ImGui::Text("激光启动 : %d", temp4.MW10.laser_on);

			ImGui::SameLine();
			ImGui::Text("有电: %d", temp4.MW13_MW14.激光器有电信号);

			ImGui::SameLine();
			ImGui::Text("指示光: %d", temp4.MW10.laser_indicator);

			ImGui::SameLine();
			ImGui::Text("激光使能: %d", temp4.MW10.laser_enabled);
			//if (ImGui::Button("SACN_TEST1")) {
			//	func.funCode = 8;
			//	taskPush(func);
			//}

			//if (ImGui::Button("SACN_TEST2")) {
			//	func.funCode = 12;
			//	taskPush(func);
			//}
			ImGui::InputFloat("##工作台位移大小", &pb_dis_);
			ImGui::SameLine();
			if (ImGui::Button("Z轴移动")) {

				Axis_Move(Z, pb_dis_);
			}


			ImGui::InputFloat("##供粉缸位移大小", &feed_dis_);
			ImGui::SameLine();
			if (ImGui::Button("F轴移动")) {
				Axis_Move(F, feed_dis_);
			}


			ImGui::InputFloat("##铺粉缸位移大小", &rom_dis_);
			ImGui::SameLine();
			if (ImGui::Button("C轴移动")) {
				Axis_Move(C, rom_dis_);
			}
			ImGui::SameLine();
			if (ImGui::Button("C轴回零")) {
				Axis_Move(C, 0);
			}


			ImGui::Text("Z轴 = %.2f", temp4.Z轴当前位置 / Motor_ratio[0]);
			ImGui::Text("F轴 = %.2f", 108 - temp4.F轴当前位置 / Motor_ratio[1]);
			ImGui::Text("C轴 = %.2f", temp4.C轴当前位置 / Motor_ratio[2]);
			ImGui::Text("氧气含量高精度：%.2f", temp4.氧含量高精度 / 100.0f);
			ImGui::Text("氧气含量低精度：%.2f", temp4.氧含量低精度 / 100.0f);
			ImGui::Text("腔体压力（Pa）：% .1f", temp4.腔体压力 / 1.0f);
			if (ImGui::Button("风机供电")) {
				out_data.MW10.fan_power = ~temp4.MW10.fan_power;
				rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
				ModbusTaskPush(rs);
			}
			ImGui::SameLine();
			if (ImGui::Button("风机启动")) {
				out_data.MW10.fan_on = ~temp4.MW10.fan_on;
				rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
				ModbusTaskPush(rs);
			}
			ImGui::SameLine();
			if (ImGui::Button("大通气阀")) {
				out_data.MW10.big_valve = ~temp4.MW10.big_valve;
				rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
				ModbusTaskPush(rs);
			}
			ImGui::SameLine();
			if (ImGui::Button("小通气阀")) {
				out_data.MW10.small_valve = ~temp4.MW10.small_valve;
				rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
				ModbusTaskPush(rs);
			}
			ImGui::SameLine();
			if (ImGui::Button("系统排气")) {
				out_data.MW10.ventilate = ~temp4.MW10.ventilate;
				rs.change(1, 10, 1, (uint16_t*)&out_data.MW10);
				ModbusTaskPush(rs);
			}
			ImGui::Text("风机供电 %d", temp4.MW10.fan_power);

			ImGui::SameLine();
			ImGui::Text("风机开启 %d", temp4.MW10.fan_on);

			ImGui::SameLine();
			ImGui::Text("大通气阀 %d", temp4.MW10.big_valve);
			ImGui::SameLine();
			ImGui::Text("小通气阀 %d", temp4.MW10.small_valve);
			ImGui::SameLine();
			ImGui::Text("系统排气 %d", temp4.MW10.ventilate);
/*			if (ImGui::Button("Manufacturing")) {
				func.funCode = 15;
				taskPush(func);
			}*///todo funcode增加一个bit位
			//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.

		// Rendering
		ImGui::Render();
		glViewport(0, 0, g_Width, g_Height);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Present
		::SwapBuffers(g_MainWindow.hDC);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	CleanupDeviceWGL(hwnd, &g_MainWindow);
	wglDeleteContext(g_hRC);
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}


// Helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
	HDC hDc = ::GetDC(hWnd);
	PIXELFORMATDESCRIPTOR pfd = { 0 };
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;

	const int pf = ::ChoosePixelFormat(hDc, &pfd);
	if (pf == 0)
		return false;
	if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
		return false;
	::ReleaseDC(hWnd, hDc);

	data->hDC = ::GetDC(hWnd);
	if (!g_hRC)
		g_hRC = wglCreateContext(data->hDC);
	return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
	wglMakeCurrent(NULL, NULL);
	::ReleaseDC(hWnd, data->hDC);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			g_Width = LOWORD(lParam);
			g_Height = HIWORD(lParam);
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
