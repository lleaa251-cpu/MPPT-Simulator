#ifndef DIAGNOSIS_H
#define DIAGNOSIS_H

// ============================================================
//  diagnosis.h — 太陽能板損耗診斷模組
//
//  原理：在相同 G、T 條件下，比對理論值與實測值的偏差率
//        不同損耗類型會造成不同參數的偏差組合
// ============================================================

#include <string>
#include <vector>

// 診斷輸入：使用者實測的三個數值
struct MeasuredData {
    double Voc_m;   // 實測開路電壓 (V)
    double Isc_m;   // 實測短路電流 (A)
    double Pmpp_m;  // 實測最大功率 (W)
};

// 單一診斷結果
struct DiagnosisItem {
    std::string parameter;   // 參數名稱（如 "Isc 偏差率"）
    double theoretical;      // 理論值
    double measured;         // 實測值
    double deviation_pct;    // 偏差率 (%)
    bool   abnormal;         // 是否異常（偏差 > 閾值）
};

// 整體診斷報告
struct DiagnosisReport {
    std::vector<DiagnosisItem> items;   // 各參數比對結果
    std::string conclusion;             // 主要診斷結論
    std::string suggestion;             // 建議動作
    bool healthy;                       // true = 正常，false = 異常
};

// ── 閾值設定（可依需求調整）────────────────────────────────
//   Isc 偏差 > 10% → 光衰減 / 老化
//   Voc 偏差 > 5%  → 嚴重損傷（接面問題）
//   FF  偏差 > 8%  → 髒污遮蔽 / 串聯電阻增大
constexpr double THRESH_ISC = 10.0;
constexpr double THRESH_VOC =  5.0;
constexpr double THRESH_FF  =  8.0;

// 執行診斷
// 參數：理論值（從 MPPTResult 取得）、實測值、天氣條件（G, T）
DiagnosisReport runDiagnosis(
    double Voc_th,    // 理論 Voc (V)
    double Isc_th,    // 理論 Isc (A)
    double Pmpp_th,   // 理論 Pmpp (W)
    double FF_th,     // 理論 FF
    const MeasuredData& measured
);

// 印出診斷報告（終端機）
void printDiagnosisReport(const DiagnosisReport& report, double G, double T_C);

// 匯出診斷報告至 CSV
bool exportDiagnosisCSV(const DiagnosisReport& report,
                        double G, double T_C,
                        const std::string& filename);

#endif
