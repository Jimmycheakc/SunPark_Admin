//File: parsedata.h
//Description: Functions to parse data
//Author: Yan Naing Aye
//MIT License - Copyright (c) 2018 Yan Naing Aye
#ifndef PARSEDATA_H_INCLUDED
#define PARSEDATA_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <vector>
using namespace std;
//---------------------------------------------------------
//Parse JSON

using namespace std;
class ParseData {
    vector<string> field;
    char start_ch;
    char end_ch;
    char separator;
public:
    ParseData();
    ParseData(char startChar,char endChar,char separatorChar);
    int Parse(string str);
    string Field(int number);
    void SetStyle(char startChar,char endChar,char separatorChar);
    static string SetStrLen(string str,int n);
    static string i2nc(int i,int n);//convert an integer into string of n character
};

//truncate and take last n char
//  or pad '0' in front
//  to get a string of defined len
string ParseData::SetStrLen(string str,int n)
{
    string ostr=string(n,'0');
    ostr+=str;
    ostr=ostr.substr(ostr.length()-n,n);
    return ostr;
}

//convert an integer into string of n character
string ParseData::i2nc(int i,int n)
{
    string ostr=string(n,'0');
    ostr+=to_string(i);
    ostr=ostr.substr(ostr.length()-n,n);
    return ostr;
}
//---------------------------------------------------------
ParseData::ParseData()
{
    SetStyle('[',']','|');
}

ParseData::ParseData(char startChar,char endChar,char separatorChar)
{
    SetStyle(startChar,endChar,separatorChar);
}

void ParseData::SetStyle(char startChar,char endChar,char separatorChar)
{
    start_ch=startChar;
    end_ch=endChar;
    separator=separatorChar;
}
int ParseData::Parse(string str)
{
    field.clear();

    int startpos=str.find(start_ch);

    // if no start, to take from start, comment the following line
    //if(startpos<0) return field.size();

    startpos++;//not taking start_ch

    int endpos=str.find(end_ch,startpos);
    //if(endpos<0) return field.size();//if no end, discard all
    if(endpos<0) endpos=str.length();//if no end, take all


    int len=endpos-startpos;
    if(len<=0) return field.size();
    str=str.substr(startpos,len);

    //cout<<"Data mes"<<str<<endl;
    while(1){
        startpos=str.find(separator);
        if(startpos>=0){
            field.push_back(str.substr(0,startpos));
            startpos++;
            len=str.length();
            if(startpos>=len){
                break;
            }
            else {
                str=str.substr(startpos);
            }
        }
        else{
            if(str.length()>0){
                field.push_back(str);
            }
            break;//end the loop
        }
    }
    return field.size();
}

string ParseData::Field(int number)
{
    if(number<field.size()) return field.at(number);
    else return "";
}


#endif // PARSEDATA_H_INCLUDED
