#pragma once
#include "fmt/format.h"
#include <iomanip>
#include <sstream>
#include <string>


using string = std::string;

namespace land {

struct Date {
private:
    std::tm mTm{};

    std::tm _now() {
        std::time_t t = std::time(nullptr);
        std::tm     now;
        localtime_s(&now, &t);
        return now;
    }

public:
    [[nodiscard]] int getYear(int year = 0) const { return mTm.tm_year + 1900 + year; }
    bool              setYear(int year) { return mTm.tm_year = year - 1900; }
    [[nodiscard]] int getMonth() const { return mTm.tm_mon + 1; }
    bool              setMonth(int month) { return mTm.tm_mon = month - 1; }
    [[nodiscard]] int getDay() const { return mTm.tm_mday; }
    bool              setDay(int day) { return mTm.tm_mday = day; }
    [[nodiscard]] int getHour() const { return mTm.tm_hour; }
    bool              setHour(int hour) { return mTm.tm_hour = hour; }
    [[nodiscard]] int getMinute() const { return mTm.tm_min; }
    bool              setMinute(int minute) { return mTm.tm_min = minute; }
    [[nodiscard]] int getSecond() const { return mTm.tm_sec; }
    bool              setSecond(int second) { return mTm.tm_sec = second; }

    std::time_t getTime() const {
        std::tm tm = mTm;
        return std::mktime(&tm);
    }

    [[nodiscard]] string toString() const {
        return fmt::format(
            "{}-{}-{} {}:{}:{}",
            std::to_string(getYear()),
            std::to_string(getMonth()),
            std::to_string(getDay()),
            std::to_string(getHour()),
            std::to_string(getMinute()),
            std::to_string(getSecond())
        );
    }

    // Date constructors
    Date(std::tm tm) : mTm(tm) {}
    Date() { mTm = _now(); }
    Date(int year, int month, int day, int hour = 0, int minute = 0, int second = 0) {
        mTm.tm_year = year - 1900;
        mTm.tm_mon  = month - 1;
        mTm.tm_mday = day;
        mTm.tm_hour = hour;
        mTm.tm_min  = minute;
        mTm.tm_sec  = second;
    }

    // Static methods
    static Date now() { return Date{}; }
    static Date future(int seconds) {
        std::time_t now    = std::time(nullptr);
        std::time_t future = now + seconds;
        std::tm     tm{};
        localtime_s(&tm, &future);
        return Date{tm};
    }
    static Date parse(const string& str) {
        // 字符串格式: "YYYY-MM-DD HH:MM:SS"
        std::istringstream iss(str);
        string             datePart, timePart;
        std::getline(iss, datePart, ' ');
        std::getline(iss, timePart);

        std::tm tm = {};
        int     year, month, day, hour, minute, second;

        sscanf_s(datePart.c_str(), "%d-%d-%d", &year, &month, &day);     // 解析日期部分
        sscanf_s(timePart.c_str(), "%d:%d:%d", &hour, &minute, &second); // 解析时间部分

        tm.tm_year = year - 1900;
        tm.tm_mon  = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min  = minute;
        tm.tm_sec  = second;

        return Date(tm);
    }
    static Date clone(Date& d) { return Date(d.mTm); }

    // 重载操作符
    Date& operator=(const Date& other) {
        if (this != &other) {
            mTm = other.mTm;
        }
        return *this;
    }
    bool operator==(const Date& other) const {
        return getYear() == other.getYear() && getMonth() == other.getMonth() && getDay() == other.getDay()
            && getHour() == other.getHour() && getMinute() == other.getMinute() && getSecond() == other.getSecond();
    }
    bool operator<(const Date& other) const {
        if (getYear() != other.getYear()) return getYear() < other.getYear();
        if (getMonth() != other.getMonth()) return getMonth() < other.getMonth();
        if (getDay() != other.getDay()) return getDay() < other.getDay();
        if (getHour() != other.getHour()) return getHour() < other.getHour();
        if (getMinute() != other.getMinute()) return getMinute() < other.getMinute();
        return getSecond() < other.getSecond();
    }
    bool        operator>(const Date& other) const { return other < *this; }
    bool        operator<=(const Date& other) const { return !(*this > other); }
    bool        operator>=(const Date& other) const { return !(*this < other); }
    bool        operator!=(const Date& other) const { return !(*this == other); }
    std::time_t operator-(const Date& other) const { return (this->getTime() - other.getTime()); }
};

} // namespace land