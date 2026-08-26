#pragma once

// نگاشت نام‌های استاندارد IEC 61131-3 به نام‌های داخلی زبان QPLC
// و دسته‌بندی توابع داخلی (تایمر/شمارنده/لبه)
#include <string>

namespace builtins {

// نام استاندارد IEC → نام داخلی (نام‌های قدیمی بدون تغییر پاس می‌شوند)
inline std::string normalize(const std::string& name) {
    if (name == "TON")  return "on_delay";
    if (name == "TOF")  return "off_delay";
    if (name == "TP")   return "pulse";
    if (name == "CTU")  return "count_up";
    if (name == "CTD")  return "count_down";
    if (name == "CTUD") return "count_updown";
    return name;
}

inline bool isTimer(const std::string& canonical) {
    return canonical == "on_delay" || canonical == "off_delay" || canonical == "pulse";
}

inline bool isCounter(const std::string& canonical) {
    return canonical == "count_up" || canonical == "count_down" || canonical == "count_updown";
}

inline bool isEdge(const std::string& canonical) {
    return canonical == "rising_edge" || canonical == "falling_edge";
}

}  // namespace builtins
