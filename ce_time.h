//File: ce_time.h
//Description: Simple Date/time class
//WebSite: http://cool-emerald.blogspot.com
//MIT License (https://opensource.org/licenses/MIT)
//Copyright (c) 2017 Yan Naing Aye

#ifndef CE_TIME_H
#define CE_TIME_H

#include<stdio.h>
#include<string>
#include<time.h>
#include<math.h>

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(__WINDOWS__) || defined(__TOS_WIN__)
    #define CE_WINDOWS
    #include <windows.h>
#else
    #define CE_LINUX
    #include <sys/time.h>
#endif // defined


using namespace std;

class CE_Time
{
private:
    double t;
    double time_zone;
    string ChkNumStr(string str);
public:
	int weekday[7] = {6,7,1,2,3,4,5};
    CE_Time();//default constructor
    CE_Time(double a);//constructor to init
    CE_Time(long year, long month, long day, long hour, long minute, double second);//constructor to init

	static double g2j(long year, long month, long day, long hour, long minute, double second);//convert to julian date
	static void j2g(double jd,long& year, long& month, long& day, long& hour, long& minute, double& second);//convert to Gregorian date
    static double u2j(time_t ut);//unix timestamp to julian date
    static double Now();
    static double SystemTimeZone();
	double SetTimeZone(double a);//set time zone
	double SetTimeZone();//set time zone
	double GetTimeZone();//get time zone
	double SetTime();//set to current time
    double SetTime(double a);//set time in JD
    double SetTime(long year, long month, long day, long hour, long minute, double second);
    double SetTime(time_t ut);
    double SetTime(string tstr);
    double SetTime(string tstr,long& sucFlag);
    double JD();//get time in JD
    double Period(double a);
    double Period(long year, long month, long day, long hour, long minute, double second);
	double Period();
    long Year();
    long Month();
    long Day();
    long Hour();
    long Minute();
    double Second();
	int getweekday();
    time_t GetUnixTimestamp();
	int diffday(time_t qt1, time_t qt2);
	int diffhour(time_t qt1, time_t qt2);
	int diffmin(time_t qt1, time_t qt2);

    double getMs();//2019.07.22 QC

	string TimeString();
	string TimeWithMsString();
	string DateString();
	string Datestr();
    string DateTimeString();
    string DateTimeNumberOnlyString();
	CE_Time(string tstr);
	static string compiletime();
	static void SetSystemLocalTime(int year, int month, int day,int hour, int minute, int second);
};


#endif
