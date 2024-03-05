#pragma once

#include <memory>
#include <string>

class IniParser
{

public:
    const std::string INI_FILE = "LinuxPBS.ini";

    static IniParser* getInstance();
    void FnReadIniFile();
    void FnPrintIniFile();
    std::string FnGetStationID() const;
    std::string FnGetCentralDBServer() const;

    /**
     * Singleton IniParser should not be cloneable.
     */
    IniParser(IniParser &iniparser) = delete;

    /**
     * Singleton IniParser should not be assignable.
     */
    void operator=(const IniParser &) = delete;

private:
    static IniParser* iniParser_;
    IniParser();

    std::string StationID_;
    std::string CentralDBServer_;
};