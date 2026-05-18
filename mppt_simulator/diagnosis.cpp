#include "diagnosis.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <algorithm>

// ============================================================
//  diagnosis.cpp — 損耗診斷實作
// ============================================================

// ----------------------------------------------------------
//  偏差率計算（正值 = 實測低於理論，即「損失」）
// ----------------------------------------------------------
static double deviationPct(double theoretical, double measured) {
    if (std::fabs(theoretical) < 1e-9) return 0.0;
    return (theoretical - measured) / theoretical * 100.0;
}

// ----------------------------------------------------------
//  runDiagnosis — 核心診斷邏輯
//
//  損耗特徵矩陣：
//  ┌─────────────────┬──────────┬──────────┬──────────┐
//  │ 損耗類型         │ ΔIsc     │ ΔVoc     │ ΔFF      │
//  ├─────────────────┼──────────┼──────────┼──────────┤
//  │ 光衰減 / 老化    │ 大       │ 小       │ 小       │
//  │ 髒污 / 遮蔽      │ 中       │ 小       │ 大       │
//  │ 焊點腐蝕（Rs↑）  │ 小       │ 小       │ 大       │
//  │ 接面損傷（I0↑）  │ 小       │ 大       │ 中       │
//  │ 正常（天氣波動） │ 小       │ 小       │ 小       │
//  └─────────────────┴──────────┴──────────┴──────────┘
// ----------------------------------------------------------
DiagnosisReport runDiagnosis(
    double Voc_th, double Isc_th, double Pmpp_th, double FF_th,
    const MeasuredData& m)
{
    DiagnosisReport report;

    // 計算實測 FF
    double FF_m = (m.Voc_m > 0 && m.Isc_m > 0)
                  ? m.Pmpp_m / (m.Voc_m * m.Isc_m)
                  : 0.0;

    // 各參數偏差率
    double dIsc  = deviationPct(Isc_th,  m.Isc_m);
    double dVoc  = deviationPct(Voc_th,  m.Voc_m);
    double dFF   = deviationPct(FF_th,   FF_m);
    double dPmpp = deviationPct(Pmpp_th, m.Pmpp_m);

    // 建立各項目
    report.items.push_back({"Isc（短路電流）", Isc_th,  m.Isc_m,  dIsc,  dIsc  > THRESH_ISC});
    report.items.push_back({"Voc（開路電壓）", Voc_th,  m.Voc_m,  dVoc,  dVoc  > THRESH_VOC});
    report.items.push_back({"FF（填充因子）",  FF_th,   FF_m,     dFF,   dFF   > THRESH_FF });
    report.items.push_back({"Pmpp（最大功率）",Pmpp_th, m.Pmpp_m, dPmpp, false });  // 僅供參考

    bool isc_abn = dIsc > THRESH_ISC;
    bool voc_abn = dVoc > THRESH_VOC;
    bool ff_abn  = dFF  > THRESH_FF;

    // ── 診斷邏輯 ────────────────────────────────────────────
    if (!isc_abn && !voc_abn && !ff_abn) {
        report.healthy    = true;
        report.conclusion = "正常：各參數偏差在允許範圍內";
        report.suggestion = "數值波動可能來自天氣變化（G、T 的量測誤差），"
                            "建議在穩定日照下重測確認。";

    } else if (isc_abn && !voc_abn && !ff_abn) {
        report.healthy    = false;
        report.conclusion = "異常：光衰減 / 電池老化";
        report.suggestion = "Isc 顯著下降而 Voc 正常，為電池光電轉換效率退化的典型特徵。"
                            "建議：確認電池板使用年限，考慮更換老化模組。";

    } else if (isc_abn && !voc_abn && ff_abn) {
        report.healthy    = false;
        report.conclusion = "異常：表面髒污 / 局部遮蔽";
        report.suggestion = "Isc 與 FF 同時下降，Voc 正常，符合灰塵或鳥糞遮蔽特徵。"
                            "建議：清潔電池板表面，特別注意局部遮蔽點。";

    } else if (!isc_abn && !voc_abn && ff_abn) {
        report.healthy    = false;
        report.conclusion = "異常：串聯電阻增大（焊點腐蝕 / 接線老化）";
        report.suggestion = "FF 明顯下降但 Isc、Voc 正常，為 Rs 增大的特徵（高電壓端電流損失）。"
                            "建議：檢查接線端子與焊點，測量各串聯迴路電阻。";

    } else if (!isc_abn && voc_abn) {
        report.healthy    = false;
        report.conclusion = "異常：P-N 接面損傷（I0 增大）";
        report.suggestion = "Voc 顯著下降為接面損傷的特徵，常見於雷擊、過電壓或長期高溫。"
                            "建議：進行 EL（電致發光）影像檢測，確認損傷位置。";

    } else {
        report.healthy    = false;
        report.conclusion = "異常：複合型損耗（多種因素同時存在）";
        report.suggestion = "多個參數同時偏差，建議進行完整電性量測（I-V curve tracer）"
                            "並與廠商 datasheet 比對，確認損耗來源。";
    }

    return report;
}

// ----------------------------------------------------------
//  printDiagnosisReport — 終端機輸出
// ----------------------------------------------------------
void printDiagnosisReport(const DiagnosisReport& report, double G, double T_C) {
    auto sep = [](char c = '=', int w = 56) {
        std::cout << std::string(w, c) << "\n";
    };

    sep();
    std::cout << "  太陽能板健康診斷報告\n";
    sep();
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  量測條件：G = " << G << " W/m2，T = " << T_C << " °C\n";
    sep('-');

    // 參數比對表格
    std::cout << "  " << std::left
              << std::setw(18) << "參數"
              << std::setw(10) << "理論值"
              << std::setw(10) << "實測值"
              << std::setw(10) << "偏差率"
              << "狀態\n";
    sep('-');

    for (const auto& item : report.items) {
        std::cout << "  " << std::left << std::setw(18) << item.parameter
                  << std::setw(10) << std::fixed << std::setprecision(3) << item.theoretical
                  << std::setw(10) << item.measured
                  << std::setw(8)  << std::setprecision(1) << item.deviation_pct << "%"
                  << (item.abnormal ? "  [!] 異常" : "  [OK]")
                  << "\n";
    }

    sep('-');
    std::cout << "\n  【診斷結論】" << report.conclusion << "\n\n";
    std::cout << "  【建議動作】" << report.suggestion  << "\n\n";

    if (report.healthy)
        std::cout << "  整體狀態：[正常] 電池板運作良好\n";
    else
        std::cout << "  整體狀態：[需注意] 建議進一步檢查\n";

    sep();
}

// ----------------------------------------------------------
//  exportDiagnosisCSV — 匯出診斷結果
// ----------------------------------------------------------
bool exportDiagnosisCSV(const DiagnosisReport& report,
                        double G, double T_C,
                        const std::string& filename)
{
    std::ofstream ofs(filename);
    if (!ofs.is_open()) return false;

    ofs << "# 太陽能板健康診斷報告\n";
    ofs << "# G(W/m2)," << G << ",T(degC)," << T_C << "\n";
    ofs << "# 結論," << report.conclusion << "\n";
    ofs << "# 建議," << report.suggestion << "\n#\n";
    ofs << "參數,理論值,實測值,偏差率(%),狀態\n";

    for (const auto& item : report.items) {
        ofs << item.parameter << ","
            << item.theoretical << ","
            << item.measured    << ","
            << item.deviation_pct << ","
            << (item.abnormal ? "異常" : "正常") << "\n";
    }

    ofs.close();
    return true;
}
