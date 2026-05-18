#include "dynamic.h"
#include "iv_curve.h"
#include "mppt.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <stdexcept>

// ============================================================
//  dynamic.cpp — 動態天氣模擬 + P&O 追蹤演算法實作
// ============================================================

// ----------------------------------------------------------
//  預設天氣場景定義
//  每個場景由若干 (時間, 日照) 關鍵點組成，中間線性內插
// ----------------------------------------------------------
std::vector<GPoint> getWeatherProfile(WeatherEvent event) {
    switch (event) {

    case WeatherEvent::CLOUD_PASS:
        // 烏雲飄過：穩定晴天 → 烏雲遮蔽 → 恢復晴天
        return {
            { 0.0,  1000.0},   // 晴天
            {10.0,  1000.0},   // 維持
            {14.0,   200.0},   // 烏雲快速遮蔽（4 秒下降）
            {40.0,   200.0},   // 烏雲覆蓋中
            {44.0,  1000.0},   // 烏雲散去（4 秒回升）
            {60.0,  1000.0},   // 恢復穩定
        };

    case WeatherEvent::SUNRISE:
        // 日出：清晨低日照緩慢上升至正午
        return {
            { 0.0,   80.0},
            {10.0,  200.0},
            {25.0,  450.0},
            {45.0,  750.0},
            {60.0, 1000.0},
        };

    case WeatherEvent::SUNSET:
        // 日落：正午逐漸下降至黃昏
        return {
            { 0.0, 1000.0},
            {15.0,  750.0},
            {35.0,  450.0},
            {50.0,  200.0},
            {60.0,   80.0},
        };

    case WeatherEvent::INTERMITTENT_CLOUD:
        // 間歇性雲：多次遮蔽，測試 P&O 重複追蹤能力
        return {
            { 0.0,  1000.0},
            { 8.0,  1000.0},
            {10.0,   300.0},   // 第一次遮蔽
            {15.0,   300.0},
            {17.0,  1000.0},   // 恢復
            {23.0,  1000.0},
            {25.0,   500.0},   // 第二次（半遮蔽）
            {32.0,   500.0},
            {34.0,  1000.0},   // 恢復
            {40.0,  1000.0},
            {42.0,   150.0},   // 第三次（濃雲）
            {48.0,   150.0},
            {50.0,  1000.0},   // 最終恢復
            {60.0,  1000.0},
        };

    default:
        // CUSTOM 或其他：回傳空（由外部提供）
        return {};
    }
}

// ----------------------------------------------------------
//  線性內插：由關鍵點求任意時間 t 的 G 值
// ----------------------------------------------------------
double interpolateG(const std::vector<GPoint>& profile, double t) {
    if (profile.empty()) return 1000.0;
    if (t <= profile.front().t) return profile.front().G;
    if (t >= profile.back().t)  return profile.back().G;

    for (size_t i = 1; i < profile.size(); ++i) {
        if (t <= profile[i].t) {
            double t0 = profile[i-1].t, G0 = profile[i-1].G;
            double t1 = profile[i  ].t, G1 = profile[i  ].G;
            double ratio = (t - t0) / (t1 - t0);
            return G0 + ratio * (G1 - G0);
        }
    }
    return profile.back().G;
}

// ----------------------------------------------------------
//  P&O 演算法（Perturb & Observe）動態追蹤
//
//  每個時間步：
//   1. 由天氣曲線取得當前 G(t)
//   2. 計算目前電壓 V 下的功率 P_now
//   3. 比較 P_now 與上一步 P_prev
//      - P_now > P_prev：繼續往同方向擾動
//      - P_now < P_prev：反向擾動
//   4. 更新 V，限制在 [V_min, V_max]
//   5. 計算理論 Pmpp（同 G、T 下的真實最大值）作為對照
// ----------------------------------------------------------
std::vector<TrackingStep> runDynamicMPPT(
    const SolarCell& cell,
    WeatherEvent weather_event,
    const DynamicConfig& config,
    const std::vector<GPoint>& custom_points)
{
    // 取得日照曲線
    std::vector<GPoint> profile =
        (weather_event == WeatherEvent::CUSTOM)
        ? custom_points
        : getWeatherProfile(weather_event);

    if (profile.empty())
        throw std::runtime_error("日照曲線為空，請提供自訂關鍵點。");

    double t_end = profile.back().t;
    std::vector<TrackingStep> steps;
    steps.reserve(static_cast<int>(t_end / config.dt) + 2);

    double V       = config.V_init;   // 當前追蹤電壓
    double P_prev  = 0.0;             // 上一步功率
    double dV_dir  = config.dV;       // 擾動方向（正/負）

    for (double t = 0.0; t <= t_end + 1e-9; t += config.dt) {
        double G   = interpolateG(profile, t);
        double T_C = config.T_C;

        // 計算目前 V 下的電流與功率
        double I     = cell.calcCurrent(V, G, T_C);
        double P_now = V * I;

        // ── P&O 決策 ─────────────────────────────────────
        double dP = P_now - P_prev;

        if (std::fabs(dP) < 0.01) {
            // 功率幾乎不變 → 維持方向（穩定區）
        } else if (dP > 0) {
            // 功率上升 → 繼續同方向擾動
            // (dV_dir 不變)
        } else {
            // 功率下降 → 反向
            dV_dir = -dV_dir;
        }

        P_prev = P_now;
        V += dV_dir;
        V = std::max(config.V_min, std::min(config.V_max, V));

        // ── 計算理論最大功率（用於追蹤誤差）────────────────
        auto curve = scanIVCurve(cell, G, T_C, 150);
        MPPTResult theoretical = findMPP(curve, G, cell.A_cell);
        double P_th = theoretical.Pmpp;

        double tracking_err = (P_th > 1e-6)
                              ? std::fabs(P_th - P_now) / P_th * 100.0
                              : 0.0;

        // ── 狀態描述 ────────────────────────────────────────
        std::string status;
        if (tracking_err < 3.0)
            status = "穩定追蹤";
        else if (tracking_err < 15.0)
            status = "收斂中..";
        else if (G < 400.0)
            status = "低日照追蹤";
        else
            status = "大幅偏移!";

        steps.push_back({t, G, T_C, V, I, P_now, P_th, tracking_err, status});
    }

    return steps;
}

// ----------------------------------------------------------
//  printDynamicResult — 逐步追蹤結果表格
//  為避免輸出過多，每 2 秒印一行
// ----------------------------------------------------------
void printDynamicResult(const std::vector<TrackingStep>& steps,
                        WeatherEvent event)
{
    auto sep = [](char c = '=', int w = 72) {
        std::cout << std::string(w, c) << "\n";
    };

    sep();
    std::cout << "  P&O 動態追蹤結果 — " << getEventName(event) << "\n";
    sep();
    std::cout << "  " << std::left
              << std::setw(7)  << "時間(s)"
              << std::setw(10) << "日照(W/m2)"
              << std::setw(10) << "電壓(V)"
              << std::setw(10) << "功率(W)"
              << std::setw(12) << "理論Pmpp(W)"
              << std::setw(10) << "誤差(%)"
              << "狀態\n";
    sep('-', 72);

    // 每隔約 2 秒印一行（避免終端機爆炸）
    double print_interval = 2.0;
    double next_print = 0.0;

    for (const auto& s : steps) {
        if (s.t < next_print - 1e-9) continue;
        next_print = s.t + print_interval;

        std::cout << std::fixed
                  << "  " << std::setw(6)  << std::setprecision(1) << s.t
                  << "  " << std::setw(9)  << std::setprecision(0) << s.G
                  << "  " << std::setw(8)  << std::setprecision(2) << s.V
                  << "  " << std::setw(8)  << std::setprecision(2) << s.P
                  << "  " << std::setw(10) << std::setprecision(2) << s.P_theoretical
                  << "  " << std::setw(7)  << std::setprecision(1) << s.tracking_error_pct << "%"
                  << "  " << s.status << "\n";
    }

    sep();

    // 統計摘要
    double avg_err = 0.0;
    double max_err = 0.0;
    for (const auto& s : steps) {
        avg_err += s.tracking_error_pct;
        max_err = std::max(max_err, s.tracking_error_pct);
    }
    if (!steps.empty()) avg_err /= steps.size();

    std::cout << "\n  追蹤統計摘要：\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "    平均追蹤誤差：" << avg_err << " %\n";
    std::cout << "    最大追蹤誤差：" << max_err << " %\n";

    // 找收斂時間（誤差降回 5% 以下後維持的第一個點）
    bool found = false;
    for (size_t i = 1; i < steps.size(); ++i) {
        if (steps[i-1].tracking_error_pct > 5.0 &&
            steps[i  ].tracking_error_pct <= 5.0) {
            std::cout << "    日照驟變後收斂時間：約 "
                      << steps[i].t << " s（誤差降至 5% 以下）\n";
            found = true;
        }
    }
    if (!found)
        std::cout << "    追蹤誤差全程穩定在 5% 以下。\n";
    std::cout << "\n";
    sep();
}

// ----------------------------------------------------------
//  printDynamicChart — ASCII 動態追蹤圖
//  縱軸：功率（W），橫軸：時間（s）
//  兩條線：理論 Pmpp（-）vs P&O 追蹤（*）
// ----------------------------------------------------------
void printDynamicChart(const std::vector<TrackingStep>& steps,
                       int W, int H)
{
    if (steps.empty() || W < 10 || H < 4) return;

    double t_end = steps.back().t;
    double P_max = 0.0;
    for (const auto& s : steps)
        P_max = std::max(P_max, std::max(s.P, s.P_theoretical));
    P_max *= 1.1;

    // 建立兩層網格（理論 & 追蹤）
    std::vector<std::string> grid(H, std::string(W, ' '));

    auto toCol = [&](double t) {
        return static_cast<int>(t / t_end * (W - 1));
    };
    auto toRow = [&](double p) {
        return H - 1 - static_cast<int>(p / P_max * (H - 1));
    };
    auto clamp = [](int v, int lo, int hi) {
        return std::max(lo, std::min(hi, v));
    };

    for (const auto& s : steps) {
        int c = clamp(toCol(s.t), 0, W-1);
        int r_th = clamp(toRow(s.P_theoretical), 0, H-1);
        int r_tr = clamp(toRow(s.P), 0, H-1);
        if (grid[r_th][c] == ' ') grid[r_th][c] = '-';  // 理論線
        grid[r_tr][c] = '*';                              // 追蹤線（優先）
    }

    std::cout << "\n  功率追蹤圖（- = 理論 Pmpp，* = P&O 實際追蹤）\n\n";
    for (int ri = 0; ri < H; ++ri) {
        double pLabel = P_max * (H - 1 - ri) / (H - 1);
        std::cout << "  " << std::setw(6) << std::fixed
                  << std::setprecision(0) << pLabel << "W |";
        std::cout << grid[ri] << "\n";
    }
    std::cout << "        +" << std::string(W, '-') << "\n";

    // 橫軸刻度
    std::cout << "         ";
    for (int t = 0; t <= 4; ++t) {
        std::string s = std::to_string(static_cast<int>(t_end * t / 4)) + "s";
        std::cout << std::left << std::setw(W / 4 + 2) << s;
    }
    std::cout << "\n\n";
}

// ----------------------------------------------------------
//  exportDynamicCSV — 匯出動態追蹤資料
// ----------------------------------------------------------
bool exportDynamicCSV(const std::vector<TrackingStep>& steps,
                      const std::string& filename)
{
    std::ofstream ofs(filename);
    if (!ofs.is_open()) return false;

    ofs << "# MPPT 動態追蹤模擬結果\n";
    ofs << "Time(s),G(W/m2),T(degC),V_track(V),I_track(A),"
           "P_track(W),P_theoretical(W),TrackingError(%),Status\n";

    ofs << std::fixed << std::setprecision(4);
    for (const auto& s : steps) {
        ofs << s.t           << ","
            << s.G           << ","
            << s.T_C         << ","
            << s.V           << ","
            << s.I           << ","
            << s.P           << ","
            << s.P_theoretical << ","
            << s.tracking_error_pct << ","
            << s.status      << "\n";
    }

    ofs.close();
    return true;
}

// ----------------------------------------------------------
//  getEventName — 場景名稱字串
// ----------------------------------------------------------
std::string getEventName(WeatherEvent event) {
    switch (event) {
    case WeatherEvent::CLOUD_PASS:         return "烏雲飄過（G: 1000→200→1000 W/m2）";
    case WeatherEvent::SUNRISE:            return "日出過程（G: 80→1000 W/m2）";
    case WeatherEvent::SUNSET:             return "日落過程（G: 1000→80 W/m2）";
    case WeatherEvent::INTERMITTENT_CLOUD: return "間歇性雲（多次遮蔽）";
    case WeatherEvent::CUSTOM:             return "自訂天氣曲線";
    default:                               return "未知場景";
    }
}
