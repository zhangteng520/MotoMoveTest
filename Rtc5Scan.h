#pragma once
#include <stdio.h>
#include <tchar.h>
#include <conio.h>

// RTC5 header file for implicitly linking to the RTC5DLL.DLL
#include "RTC5impl.h"
#include "clipper2.h"

struct polygon
{
	LONG xval, yval;
};


static const int m_stepRatio = 5882; 
void scanLineSupport(Clipper2Lib::Paths64& paths64, float power, float speed);
void scanContour(Clipper2Lib::Paths64& paths64,float power,float speed);
void scanFill(Clipper2Lib::Paths64& paths64,float power,float speed);
void VariableSacn(const VariPathss& path,double speed, const int commandNum);
void draw(const polygon* figure, const size_t& size);
void terminateDLL(void);
int scanInitial();
int scanInitial(const std::string& rtc_crt_file);
int scanLines(int layers);
int scanFree();
