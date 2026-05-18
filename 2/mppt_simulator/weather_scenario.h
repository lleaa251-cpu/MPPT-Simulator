#ifndef WEATHER_SCENARIO_H
#define WEATHER_SCENARIO_H

// ============================================================
//  weather_scenario.h — 預設天氣場景模組
// ============================================================

#include <string>
#include <vector>

struct Scenario {
    std::string name;   // 場景名稱
    double G;           // 日照強度 (W/m²)
    double T_C;         // 溫度 (°C)
};

// 取得所有預設場景清單
std::vector<Scenario> getPresetScenarios();

#endif
