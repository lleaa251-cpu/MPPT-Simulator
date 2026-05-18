#ifndef DYNAMIC_H
#define DYNAMIC_H

// ============================================================
//  dynamic.h — 動態天氣模擬 + P&O MPPT 追蹤演算法
//
//  模擬日照 G 隨時間變化，P&O 演算法逐步追蹤最大功率點
//  輸出每個時間步的追蹤狀態，展示收斂過程
// ============================================================

#include "solar_cell.h"
#include <vector>
#include <string>
#include <functional>

// 每個時間步的追蹤狀態
struct TrackingStep {
    double t;           // 時間 (s)
    double G;           // 當前日照 (W/m²)
    double T_C;         // 當前溫度 (°C)
    double V;           // P&O 當前追蹤電壓 (V)
    double I;           // 當前電流 (A)
    double P;           // 當前功率 (W)
    double P_theoretical; // 同條件下理論 Pmpp (W)
    double tracking_error_pct; // 追蹤誤差 (%)
    std::string status; // 狀態描述
};

// 預設天氣場景類型
enum class WeatherEvent {
    CLOUD_PASS,         // 烏雲飄過（G: 1000→200→1000）
    SUNRISE,            // 日出過程（G: 100→1000）
    SUNSET,             // 日落過程（G: 1000→100）
    INTERMITTENT_CLOUD, // 間歇性雲（多次遮蔽）
    CUSTOM              // 自訂（使用者定義關鍵點）
};

// 自訂日照曲線關鍵點
struct GPoint {
    double t;   // 時間 (s)
    double G;   // 日照強度 (W/m²)
};

// 動態模擬參數
struct DynamicConfig {
    double dt         = 0.5;    // 時間步長 (s)
    double dV         = 0.3;    // P&O 每步擾動電壓 (V)
    double V_init     = 15.0;   // 追蹤起始電壓 (V)
    double V_min      = 1.0;    // 電壓下限 (V)
    double V_max      = 22.0;   // 電壓上限 (V)
    double T_C        = 25.0;   // 固定溫度（動態模擬中溫度變化慢，簡化為固定）
};

// P&O 演算法動態追蹤
// weather_event：選擇預設場景
// custom_points：若 weather_event == CUSTOM，提供自訂日照關鍵點
std::vector<TrackingStep> runDynamicMPPT(
    const SolarCell& cell,
    WeatherEvent weather_event,
    const DynamicConfig& config = DynamicConfig(),
    const std::vector<GPoint>& custom_points = {}
);

// 取得預設場景的日照曲線
std::vector<GPoint> getWeatherProfile(WeatherEvent event);

// 由關鍵點線性內插得到時間 t 的日照值
double interpolateG(const std::vector<GPoint>& profile, double t);

// 印出動態追蹤結果表格（終端機）
void printDynamicResult(const std::vector<TrackingStep>& steps,
                        WeatherEvent event);

// 印出 ASCII 動態追蹤圖（功率追蹤 vs 時間）
void printDynamicChart(const std::vector<TrackingStep>& steps,
                       int chartWidth = 50, int chartHeight = 14);

// 匯出動態追蹤結果至 CSV
bool exportDynamicCSV(const std::vector<TrackingStep>& steps,
                      const std::string& filename);

// 取得場景名稱字串
std::string getEventName(WeatherEvent event);

#endif
