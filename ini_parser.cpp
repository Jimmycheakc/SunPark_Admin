#include <iostream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "ini_parser.h"

IniParser* IniParser::iniParser_;

IniParser::IniParser()
{

}

IniParser* IniParser::getInstance()
{
    if (iniParser_ == nullptr)
    {
        iniParser_ = new IniParser();
    }
    return iniParser_;
}

void IniParser::FnReadIniFile()
{
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(INI_FILE, pt);

    // Temp: Revisit and implement storing to private variable function
    StationID_                      = pt.get<std::string>("setting.StationID", "");
    CentralDBServer_                = pt.get<std::string>("setting.CentralDBServer", "");
}

void IniParser::FnPrintIniFile()
{
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(INI_FILE, pt);

    for (const auto&section : pt)
    {
        const auto& section_name = section.first;
        const auto& section_properties = section.second;

        std::cout << "Section: " << section_name << std::endl;

        for (const auto& key : section_properties)
        {
            const auto& key_name = key.first;
            const auto& key_value = key.second.get_value<std::string>();

            std::cout << " Key: " << key_name << ", Value: "<< key_value << std::endl;
        }
    }
}

std::string IniParser::FnGetStationID() const
{
    return StationID_;
}

std::string IniParser::FnGetCentralDBServer() const
{
    return CentralDBServer_;
}