#ifndef MPPT_H
#define MPPT_H

// ============================================================
//  mppt.h — 最大功率點追蹤（MPPT）模組
// ============================================================

#include "iv_curve.h"
#include <vector>

struct MPPTResult {
    double Vmpp;  // 最大功率點電壓 (V)
    double Impp;  // 最大功率點電流 (A)
    double Pmpp;  // 最大功率 (W)
    double Voc;   // 開路電壓 (V)
    double Isc;   // 短路電流 (A)
    double FF;    // 填充因子 Fill Factor = Pmpp / (Voc * Isc)
    double eta;   // 轉換效率 (%) = Pmpp / (G * A_cell) * 100
};

// 從 I-V 曲線資料中找出最大功率點
MPPTResult findMPP(
    const std::vector<DataPoint>& curve,
    double G,
    double A_cell = 0.01    // 電池面積 (m²)，預設 0.01 m²
);

#endif
