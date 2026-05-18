#ifndef IV_CURVE_H
#define IV_CURVE_H

// ============================================================
//  iv_curve.h — I-V 曲線掃描模組
//  從 V=0 掃到 Voc，建立資料點陣列
// ============================================================

#include "solar_cell.h"
#include <vector>

struct DataPoint {
    double V;   // 電壓 (V)
    double I;   // 電流 (A)
    double P;   // 功率 (W) = V * I
};

// 掃描 I-V 曲線，回傳資料點陣列
// steps：掃描點數（預設 200）
std::vector<DataPoint> scanIVCurve(
    const SolarCell& cell,
    double G,           // 日照強度 (W/m²)
    double T_C,         // 溫度 (°C)
    int steps = 200
);

#endif
