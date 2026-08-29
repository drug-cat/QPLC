#pragma once

// Mapping IEC 61131-3 standard names to internal QPLC language names
// and categorization of built-in functions (timer/counter/edge/math)
#include <string>

namespace builtins {

// IEC standard name -> internal name (legacy names pass through unchanged)
inline std::string normalize(const std::string& name) {
    if (name == "TON")  return "on_delay";
    if (name == "TOF")  return "off_delay";
    if (name == "TP")   return "pulse";
    if (name == "CTU")  return "count_up";
    if (name == "CTD")  return "count_down";
    if (name == "CTUD") return "count_updown";
    // Standard IEC math functions -> internal lowercase name
    if (name == "MIN")   return "min";
    if (name == "MAX")   return "max";
    if (name == "ABS")   return "abs";
    if (name == "LIMIT") return "limit";
    if (name == "SEL")   return "sel";
    if (name == "MUX")   return "mux";
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

inline bool isMath(const std::string& canonical) {
    return canonical == "min" || canonical == "max" || canonical == "abs" ||
           canonical == "limit" || canonical == "sel" || canonical == "mux" ||
           canonical == "clamp";
}

// IEC standard name for SCL output (uppercase); clamp is converted to LIMIT with argument reordering
inline std::string sclName(const std::string& canonical) {
    if (canonical == "min")   return "MIN";
    if (canonical == "max")   return "MAX";
    if (canonical == "abs")   return "ABS";
    if (canonical == "limit") return "LIMIT";
    if (canonical == "sel")   return "SEL";
    if (canonical == "mux")   return "MUX";
    if (canonical == "clamp") return "LIMIT";
    return canonical;
}

// Expected argument count; -1 means variable (mux)
inline int mathArgCount(const std::string& canonical) {
    if (canonical == "min" || canonical == "max") return 2;
    if (canonical == "abs") return 1;
    if (canonical == "limit" || canonical == "sel") return 3;
    if (canonical == "mux") return -1;   // k and at least one option
    if (canonical == "clamp") return 3;  // clamp(x, lo, hi)
    return 0;
}

}  // namespace builtins
