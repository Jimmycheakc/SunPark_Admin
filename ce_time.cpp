//File: ce_time.cpp
//Description: Simple Date/time class
//WebSite: http://cool-emerald.blogspot.com
//MIT License (https://opensource.org/licenses/MIT)
//Copyright (c) 2017 Yan Naing Aye

#include "ce_time.h"
CE_Time::CE_Time()
{
    SetTimeZone();
    SetTime();
}

CE_Time::CE_Time(double a)
{
    SetTimeZone();
    SetTime(a);
}

CE_Time::CE_Time(long y, long m, long d, long h, long n, double s)
{
    SetTimeZone();
    SetTime(y,m,d,h,n,s);
}

CE_Time::CE_Time(string a)
{
    SetTimeZone();
    SetTime(a);
}

double CE_Time::g2j(long y, long m, long d, long h, long n, double s)
{
    long a=((14-m)/12);
    y=y+4800-a;
    m=m+(12*a)-3;
    double jd=d+((153*m+2)/5)+(365*y)+(y/4);
    jd=jd-(y/100)+(y/400)-32045;
	double hd=h;
	double nd=n;
    jd+=((hd-12)/24+nd/1440+s/86400);
    return jd;
}

// convert unix timestamp to jd for a timezone
// add milliseconds too
// inputs [ ut: unix timestamp]
// output [return value]: julian date
double CE_Time::u2j(time_t ut)
{
    double jd=0;
	long long ns=ut;//number of seconds from 1970 Jan 1 00:00:00 (UTC)
	jd=2440587.5+((double)ns)/86400.0;//converte to day(/24h/60min/60sec) and to JD
// if you want to include milliseconds
double ms=0;
#ifdef CE_WINDOWS
	SYSTEMTIME wt;
	GetSystemTime(&wt);
	ms=double(wt.wMilliseconds);
#else
	timeval time;
	gettimeofday(&time,NULL);
	ms=double(time.tv_usec)/1000.0;
#endif
    jd+=ms/86400000.0;
	return jd;
}


double CE_Time::getMs()
{
    double ms=0;
    timeval time;
	gettimeofday(&time,NULL);
	ms=double(time.tv_usec)/1000.0;


	return ms;

}

void CE_Time::j2g(double jd,long& year, long& month, long& day, long& hour, long& minute, double& second)
{
	long j=long(jd+0.5);
    double jf=jd+0.5-j;
	long y,m,d,h,n;
	double s;
    j-=1721119;
    y=(long)(((double)(4*j-1))/146097);
    j=4*j-1-146097*y;
    d=(long)(((double)(j))/4);
    j=(long)((4*(double)d+3)/1461);
    d=4*d+3-1461*j;
    d=(long)(((double)d+4)/4); m=(long)((5*(double)d-3)/153); d=5*d-3-153*m;
    d=(long)(((double)d+5)/5); y=100*y+j;
    if(m<10) {m+=3;}
    else {m-=9; y=y+1;}
    jf*=24; h=(long)(jf); jf=(jf-h)*60; n=(long)(jf); s=((jf-n)*60);
	year=y;
	month=m;
	day=d;
	hour=h;
	minute=n;
	second=s;
}

double CE_Time::SetTimeZone(double a)//set time zone
{
    if(a>14 || a<(-12)){
        SetTimeZone();
    }
    else {
        time_zone=a;
    }
    return time_zone;
}

double CE_Time::SystemTimeZone()
{
    double jdu,jdl,stz;
    time_t ut = time(0);//now
	long long ns=ut;//number of seconds from 1970 Jan 1 00:00:00 (UTC)
	jdu=2440587.5+((double)ns)/86400.0;//converte to day(/24h/60min/60sec) and to JD
	//http://pubs.opengroup.org/onlinepubs/7908799/xsh/time.h.html
	struct tm * ts=localtime(&ut);
	jdl=g2j(ts->tm_year+1900,ts->tm_mon+1,ts->tm_mday,ts->tm_hour,ts->tm_min,ts->tm_sec);
    stz=(jdl-jdu)*24.0;// local and utc day difference to hours
    //round to 15 min quantization
    stz=double(floor(stz*4.0+0.5))/4.0;
    return stz;
}

double CE_Time::SetTimeZone()//set local time zone
{
    time_zone=SystemTimeZone();
    return time_zone;
}

double CE_Time::GetTimeZone()//get time zone
{
    return time_zone;
}

double CE_Time::SetTime()
{
    return SetTime(Now());
}

double CE_Time::SetTime(double jd)
{
	t=jd;
    return t;
}

/*double CE_Time::SetTime(long y, long m, long d, long h, long n, double s)
{
	if(y<0 || y>9999) y=2000;
	if(m<1 || m>12) m=1;
	if(d<1 || d>31) d=1;
	if(h<0 || h>24) h=0;
	if(n<0 || n>=60) n=0;
	if(s<0 || s>=61) s=0;//leap second also
    return SetTime(g2j(y,m,d,h,n,s));
}*/

double CE_Time::SetTime(long y, long m, long d, long h, long n, double s)
{
	if(y==0) y=0;
	else if(y<2000 || y>9999) y=2000;
	
	if(m<0 || m>12) m=0;
	if(d<1 || d>31) d=1;
	if(h<0 || h>24) h=0;
	if(n<0 || n>=60) n=0;
	if(s<0 || s>=61) s=0;//leap second also
    //return SetTime(g2j(y,m,d,h,n,s));
	if(y==0)
	{
		if(m<1 || m>12) m=1;
	};
	time_t yt=0;
	
	struct tm y2t = {0};
	double seconds;
	

	y2t.tm_hour = h;   y2t.tm_min = n; y2t.tm_sec = long(s);
	y2t.tm_year = y-1900; y2t.tm_mon = m-1; y2t.tm_mday = d;
	//printf("%d-%d-%d %d:%d:%d \n",y2t.tm_year,y2t.tm_mon,y2t.tm_mday,y2t.tm_hour,y2t.tm_min, y2t.tm_sec);
	yt=mktime(&y2t);
	//printf("time yt %ld \n",yt);
	return SetTime(yt);
}

double CE_Time::Now()
{
    time_t ut = time(0);//now
    double tz=SystemTimeZone();
	double jd=u2j(ut)+(tz/24.0);
	return jd;
}

double CE_Time::SetTime(time_t ut)
{
    t=u2j(ut)+(time_zone/24.0);
	return t;
}

/*time_t CE_Time::GetUnixTimestamp()
{
    time_t ut;
    double nd=(t-2440587.5)*86400.0;
    long long ns= (long long)(nd);//nearest integer
    ns-=(long long)((time_zone)*3600);//local time zone*60*60
    ut = (time_t)(ns);
    return ut;
}*/

time_t CE_Time::GetUnixTimestamp()
{
    time_t ut=0;
	
	struct tm y2k = {0};
	double seconds;
	

	y2k.tm_hour = Hour();   y2k.tm_min = Minute(); y2k.tm_sec = Second();
	y2k.tm_year = Year()-1900; y2k.tm_mon = Month()-1; y2k.tm_mday = Day();
	//printf("%d-%d-%d %d:%d:%d \n",y2k.tm_year,y2k.tm_mon,y2k.tm_mday,y2k.tm_hour,y2k.tm_min, y2k.tm_sec);
	ut=mktime(&y2k);
	//printf("time yt %ld \n",ut);
	//printf("time zone %f \n",time_zone);
	return ut;
}

//set time as specified in string input
// accepts following formats
// Format 1: yyyy-mm-dd hh:nn:ss
// Format 2: yyyy-mm-dd hh:nn:ss.ttt
// Format 3: yyyymmddhhnnss
// Format 4: yyyymmddhhnnssttt
// Flag 0 if ok, -1 if error
double CE_Time::SetTime(string tstr,long& sucFlag)
{
    string str,pstr;
    long y=0,m=0,d=0,h=0,n=0,s=0;
    sucFlag=-1;
    double ms;
    str=ChkNumStr(tstr);
    //printf("chk str: %s ,len: %d \n",str.c_str(),str.length());
    if(str.length()==14 || str.length()==17){
        pstr=str.substr(0,4); y=stol(pstr); //get year
        pstr=str.substr(4,2); m=stol(pstr); //get month
        pstr=str.substr(6,2); d=stol(pstr); //get day
        pstr=str.substr(8,2); h=stol(pstr); //get hour
        pstr=str.substr(10,2); n=stol(pstr); //get minute
        pstr=str.substr(12,2); s=stol(pstr); //get second
        if(str.length()==17){
            pstr=str.substr(14,3); ms=double(stol(pstr)); //get millisecond
            ms=s+ms/1000.0;
            //SetTime(y,m,d,h,n,ms);
			SetTime(y,m,d,h,n,s);
            sucFlag=0;
        }
        else {
            SetTime(y,m,d,h,n,s);
            sucFlag=0;
        }
    }
    return sucFlag;
}

double CE_Time::SetTime(string tstr)
{
    long sucFlag;
    return SetTime(tstr,sucFlag);
}

string CE_Time::ChkNumStr(string str)
{
    string ostr="";
    int len=0;
    len=str.length();
    if(len>0){
        for(int i=0;i<len;i++)
            if(str[i]>='0' && str[i]<='9') ostr += str[i];
    }
    return ostr;
}

double CE_Time::JD()
{
    return t;
}

double CE_Time::Period()
{
	double a=Now();
    return Period(a);
}

double CE_Time::Period(double a)
{
    return ((a-t)*86400.0);
}

double CE_Time::Period(long y, long m, long d, long h, long n, double s)
{
    double a=g2j(y,m,d,h,n,s);
    return Period(a);
}

string CE_Time::TimeString()
{
	long year,month,day,hour,minute;
	double second;
	j2g(t,year,month,day,hour,minute,second);
	string str="";
	long s=long(second);//shold not take round to make sure s<60
    string ts=to_string(hour);
    str+=string(2-ts.length(),'0')+ts+":";
    ts=to_string(minute);
    str+=string(2-ts.length(),'0')+ts+":";
    ts=to_string(s);
    str+=string(2-ts.length(),'0')+ts;
	return str;
}

string CE_Time::TimeWithMsString()
{
	long year,month,day,hour,minute;
	double second;
	j2g(t,year,month,day,hour,minute,second);
	string str="";
	long s=long(second);//shold not take round to make sure s<60
	long ms=long((second-double(s))*1000);
    string ts=to_string(hour);
    str+=string(2-ts.length(),'0')+ts+":";
    ts=to_string(minute);
    str+=string(2-ts.length(),'0')+ts+":";
    ts=to_string(s);
    str+=string(2-ts.length(),'0')+ts+".";
    ts=to_string(ms);
    str+=string(3-ts.length(),'0')+ts;
	return str;
}


string CE_Time::DateString()
{
	long year,month,day,hour,minute;
	double second;
	j2g(t,year,month,day,hour,minute,second);
	string str="";

    string ts=to_string(year);
    str+=string(4-ts.length(),'0')+ts+"-";
    ts=to_string(month);
    str+=string(2-ts.length(),'0')+ts+"-";
    ts=to_string(day);
    str+=string(2-ts.length(),'0')+ts;

	return str;
}

string CE_Time::Datestr()
{
	long year,month,day,hour,minute;
	double second;
	j2g(t,year,month,day,hour,minute,second);
	string str="";

    string ts=to_string(year);
    str+=string(4-ts.length(),'0')+ts;
    ts=to_string(month);
    str+=string(2-ts.length(),'0')+ts;
    ts=to_string(day);
    str+=string(2-ts.length(),'0')+ts;

	return str;
}
string CE_Time::DateTimeString()
{
    string dtStr="";

    try//2019.07.05 QC, modified for return empty string
    {
        dtStr=DateString()+" "+TimeString();

    }
    catch(...)
    {

    }
    return dtStr;//DateString()+" "+TimeString();
}

string CE_Time::DateTimeNumberOnlyString()
{
    long year,month,day,hour,minute;
	double second;
	j2g(t,year,month,day,hour,minute,second);
	string str="";

    string ts=to_string(year);
    str+=string(4-ts.length(),'0')+ts;
    ts=to_string(month);
    str+=string(2-ts.length(),'0')+ts;
    ts=to_string(day);
    str+=string(2-ts.length(),'0')+ts;

    long s=long(second);//shold not take round to make sure s<60
    ts=to_string(hour);
    str+=string(2-ts.length(),'0')+ts;
    ts=to_string(minute);
    str+=string(2-ts.length(),'0')+ts;
    ts=to_string(s);
    str+=string(2-ts.length(),'0')+ts;
	return str;
}

long CE_Time::Year()
{
	long year,month,day,hour,minute;
	double second;
	j2g(t,year,month,day,hour,minute,second);
    return year;
}

long CE_Time::Month()
{
	long year,month,day,hour,minute;
	double second;
	j2g(t,year,month,day,hour,minute,second);
    return month;
}

long CE_Time::Day()
{
	long year,month,day,hour,minute;
	double second;
	j2g(t,year,month,day,hour,minute,second);
    return day;
}

long CE_Time::Hour()
{
	long year,month,day,hour,minute;
	double second;
	j2g(t,year,month,day,hour,minute,second);
    return hour;
}

long CE_Time::Minute()
{
	long year,month,day,hour,minute;
	double second;
	j2g(t,year,month,day,hour,minute,second);
    return minute;
}

double CE_Time::Second()
{
	long year,month,day,hour,minute;
	double second;
	j2g(t,year,month,day,hour,minute,second);
    return second;
}

int CE_Time::diffday(time_t qt1, time_t qt2)
{
	int d1,d2,iRet;
	d1=int((qt1+(time_zone*3600))/86400);
	d2=int((qt2+(time_zone*3600))/86400);
	printf("d1 is %d and d2 is %d\n",d1,d2);
	iRet=d2-d1;
	printf("iret is %d\n",iRet);
	return(iRet);
}
int CE_Time::diffhour(time_t qt1, time_t qt2)
{
	int d1,d2,iRet;
	d1=int(qt1/3600);
	d2=int(qt2/3600);
	iRet=d2-d1;
	return(iRet);
}
int CE_Time::diffmin(time_t qt1, time_t qt2)
{
	int d1,d2,iRet;
	d1=int(qt1/60);
	d2=int(qt2/60);
	iRet=d2-d1;
	return(iRet);
}

int CE_Time::getweekday()
{
	long year,month,day,hour,minute;
	double second;
	j2g(t,year,month,day,hour,minute,second);
	int mon;
	if(month > 2) //zellersAlgorithm
	mon = month; //for march to december month code is same as month
	else{
		mon = (12+month); //for Jan and Feb, month code will be 13 and 14 year--; //decrease year for month Jan and Feb
	}
	int y = year % 100; //last two digit
	int c = year / 100; //first two digit
	int w = (day + floor((13*(mon+1))/5) + y + floor(y/4) + floor(c/4) + (5*c));
	w = w % 7;
	return weekday[w];
}

string CE_Time::compiletime()
{
    //2019.07.08 QC
    //Need to rebuild All at code block for the date time to
    //update
	string s=__DATE__;
    s+=" ";
    s+=__TIME__;
	//s+=" ";
    //s+=__cplusplus;
	return s;
}

// Set local date time of the system
// need super user privileges
// https://www.linuxquestions.org/questions/programming-9/c-code-to-change-date-time-on-linux-707384/
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms724936(v=vs.85).aspx
void CE_Time::SetSystemLocalTime(int year, int month, int day,int hour, int minute, int second)
{
#ifdef CE_WINDOWS
  //For Windows
  SYSTEMTIME lt;
  GetLocalTime(&lt);
  lt.wYear=year;
  lt.wMonth=month;
  lt.wDay=day;
  lt.wHour=hour;
  lt.wMinute=minute;
  lt.wSecond=second;
  SetLocalTime(&lt);
#else
  //For POSX
  time_t mytime = time(0);
  struct tm* tm_ptr = localtime(&mytime);

  if (tm_ptr)
  {
    tm_ptr->tm_mon  = month - 1;
    tm_ptr->tm_mday = day;
    tm_ptr->tm_year = year - 1900;

    tm_ptr->tm_hour = hour;
    tm_ptr->tm_min = minute;
    tm_ptr->tm_sec = second;

    const struct timeval tv = {mktime(tm_ptr), 0};
    settimeofday(&tv, 0);//need super user privileges
  }
#endif
}
