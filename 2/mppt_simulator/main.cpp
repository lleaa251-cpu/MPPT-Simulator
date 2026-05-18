#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <vector>
#include <functional>

#include "solar_cell.h"
#include "iv_curve.h"
#include "mppt.h"
#include "weather_scenario.h"
#include "output.h"
#include "diagnosis.h"
#include "dynamic.h"

void clearBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

double readDouble(const std::string& prompt, double lo, double hi) {
    double val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && val >= lo && val <= hi) { clearBuffer(); return val; }
        std::cout << "  [!] 請輸入 " << lo << " ~ " << hi << " 的數值。\n";
        clearBuffer();
    }
}

int readInt(const std::string& prompt,int lo,int hi) {
    int val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && val >= lo && val <= hi) { clearBuffer();return val; }
        std::cout << "  [!] 請輸入 " << lo << " ~ " << hi << " 的整數。\n";
        clearBuffer();
    }
}

void askExportCSV(const std::string& fname,
                  std::function<bool(const std::string&)> exportFn) {
    std::cout << "\n  是否匯出 CSV 檔案？(y/n): ";
    char ans; std::cin >> ans; clearBuffer();
    if (ans == 'y' || ans == 'Y') {
        bool ok = exportFn(fname);
        std::cout << (ok ? "  [OK] 已儲存：" + fname
                         : "  [!!] 檔案寫入失敗") << "\n";
    }
}

// ── 模式 A：靜態 MPPT ────────────────────────────────────────
void modeStatic(const SolarCell&cell) {
    auto scenes = getPresetScenarios();
    const int CUSTOM = (int)scenes.size() - 1;

    printSeparator('=');
    std::cout << "  [A] 靜態 MPPT 模擬\n";
    printSeparator('-');
    for (size_t i = 0; i < scenes.size(); ++i) {
        std::cout << "  " << scenes[i].name;
        if ((int)i < CUSTOM)
            std::cout << "  (G=" << scenes[i].G << " W/m2, T=" << scenes[i].T_C << "C)";
        std::cout << "\n";
    }
    printSeparator('-');

    int ch = readInt("  選擇場景 [1~" + std::to_string(scenes.size()) + "]: ",
                     1, (int)scenes.size());
    double G, T_C; std::string name;

    if (ch - 1 == CUSTOM) {
        G = readDouble("日照強度 G(W/m2) [50~1200]: ",50,1200);
        T_C = readDouble("環境溫度 T(°C) [0~55]:    ",0,55);
        name = "自訂條件";
    } else {
        G = scenes[ch-1].G; T_C = scenes[ch-1].T_C;
        name = scenes[ch-1].name.size() > 4 ? scenes[ch-1].name.substr(4) : scenes[ch-1].name;
    }

    std::cout << "\n  [計算中...]\n\n";
    auto curve  = scanIVCurve(cell,G,T_C,300);
    auto result = findMPP(curve,G,cell.A_cell);
    printMPPTResult(result,G,T_C,name);
    printPVChart(curve,result);

    static int cnt = 0;
    askExportCSV("mppt_static_" + std::to_string(++cnt) + ".csv",
        [&](const std::string& fn){ return exportCSV(curve, result,G,T_C,fn); });
}

// ── 模式 B：健康診斷 ─────────────────────────────────────────
void modeDiagnosis(const SolarCell& cell) {
    printSeparator('=');
    std::cout << "  [B] 太陽能板健康診斷\n";
    printSeparator('-');
    std::cout << "  步驟 1：輸入量測當下的天氣條件\n\n";

    double G   = readDouble("日照強度 G(W/m2) [50~1200]: ",50,1200);
    double T_C = readDouble("環境溫度 T(°C)  [0~55]:    ", 0,55);

    auto curve = scanIVCurve(cell,G,T_C,300);
    auto th    = findMPP(curve,G,cell.A_cell);

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "\n  【理論值（同條件下）】\n";
    std::cout << "    Voc=" << th.Voc << "V  Isc=" << th.Isc
              << "A  Pmpp=" << th.Pmpp << "W  FF=" << th.FF << "\n\n";
    std::cout << "  步驟 2：輸入實測值（無法實測可輸入模擬值測試）\n\n";

    MeasuredData m;
    m.Voc_m  = readDouble("  實測 Voc  (V) [0~25]:  ", 0,  25);
    m.Isc_m  = readDouble("  實測 Isc  (A) [0~15]:  ", 0,  15);
    m.Pmpp_m = readDouble("  實測 Pmpp (W) [0~200]: ", 0, 200);

    auto report = runDiagnosis(th.Voc, th.Isc, th.Pmpp, th.FF, m);
    std::cout << "\n";
    printDiagnosisReport(report, G, T_C);

    static int cnt = 0;
    askExportCSV("mppt_diagnosis_" + std::to_string(++cnt) + ".csv",
        [&](const std::string& fn){ return exportDiagnosisCSV(report, G, T_C, fn); });
}

// ── 模式 C：動態天氣模擬 ─────────────────────────────────────
void modeDynamic(const SolarCell& cell) {
    printSeparator('=');
    std::cout << "  [C] 動態天氣模擬 — P&O MPPT 追蹤\n";
    printSeparator('-');
    std::cout << "  [1] 烏雲飄過（G: 1000->200->1000 W/m2）\n";
    std::cout << "  [2] 日出過程（G: 80->1000 W/m2）\n";
    std::cout << "  [3] 日落過程（G: 1000->80 W/m2）\n";
    std::cout << "  [4] 間歇性雲（多次遮蔽）\n";
    std::cout << "  [5] 自訂日照曲線\n";
    printSeparator('-');

    int ch = readInt("  選擇 [1~5]: ", 1, 5);
    WeatherEvent event;
    std::vector<GPoint> custom_pts;

    if      (ch == 1) event = WeatherEvent::CLOUD_PASS;
    else if (ch == 2) event = WeatherEvent::SUNRISE;
    else if (ch == 3) event = WeatherEvent::SUNSET;
    else if (ch == 4) event = WeatherEvent::INTERMITTENT_CLOUD;
    else {
        event = WeatherEvent::CUSTOM;
        std::cout << "\n  輸入關鍵點（時間 日照），輸入 -1 結束：\n";
        while (true) {
            std::cout << "  時間 t (s) [-1=結束]: ";
            double t; std::cin >> t;
            if (t < 0) { clearBuffer(); break; }
            double g = readDouble("  G (W/m2) [50~1200]: ", 50, 1200);
            custom_pts.push_back({t, g});
        }
        if (custom_pts.size() < 2) {
            std::cout << "  [!] 關鍵點不足，改用烏雲飄過場景。\n";
            event = WeatherEvent::CLOUD_PASS;
        }
    }

    DynamicConfig config;
    std::cout << "\n  時間步長 dt（預設 0.5s，直接 Enter 略過）: ";
    { std::string s; std::getline(std::cin, s);
      if (!s.empty()) try { config.dt = std::max(0.1, std::min(2.0, std::stod(s))); } catch(...){} }
    std::cout << "  擾動步長 dV（預設 0.3V，直接 Enter 略過）: ";
    { std::string s; std::getline(std::cin, s);
      if (!s.empty()) try { config.dV = std::max(0.05, std::min(1.0, std::stod(s))); } catch(...){} }

    std::cout << "\n  [模擬中，請稍候...]\n\n";
    auto steps = runDynamicMPPT(cell, event, config, custom_pts);
    printDynamicResult(steps, event);
    printDynamicChart(steps);

    static int cnt = 0;
    askExportCSV("mppt_dynamic_" + std::to_string(++cnt) + ".csv",
        [&](const std::string& fn){ return exportDynamicCSV(steps, fn); });
}

// ── 主程式 ───────────────────────────────────────────────────
int main() {
    SolarCell cell;
    while (true) {
        std::cout << "\n";
        printSeparator('=');
        std::cout << "    MPPT 太陽能板最大功率追蹤模擬器 v2.0\n";
        printSeparator('=');
        std::cout << "  [1] 靜態 MPPT 模擬    — 計算特定天氣的最大功率點\n";
        std::cout << "  [2] 太陽能板健康診斷  — 比對實測值，判斷損耗類型\n";
        std::cout << "  [3] 動態天氣模擬      — P&O 演算法追蹤日照劇變\n";
        std::cout << "  [0] 結束\n";
        printSeparator('-');
        std::cout << "  選擇功能：";

        int ch;
        if (!(std::cin >> ch)) { clearBuffer(); continue; }
        clearBuffer();

        if      (ch == 0) { std::cout << "\n  再見！\n\n"; return 0; }
        else if (ch == 1) modeStatic(cell);
        else if (ch == 2) modeDiagnosis(cell);
        else if (ch == 3) modeDynamic(cell);
        else              std::cout << "  [!] 無效選項。\n";
    }
}
