#ifndef SOLAR_CELL_H
#define SOLAR_CELL_H

// ============================================================
//  solar_cell.h — 單二極體模型（Single-Diode Model）
//  計算給定電壓V下的電流I（牛頓法迭代）
// ============================================================

struct SolarCell {
    // 標準測試條件（STC）參數
    double Isc_ref = 8.5;      // 短路電流 (A)  @ 1000 W/m², 25°C
    double Voc_ref = 21.5;     // 開路電壓 (V)
    double Ki      = 0.0005;   // 電流溫度係數 (A/°C)
    double Kv      = -0.08;    // 電壓溫度係數 (V/°C)

    // 電路等效參數
    double Rs      = 0.3;      // 串聯電阻 (Ω)
    double Rsh     = 300.0;    // 並聯電阻 (Ω)
    double n       = 1.4;      // 二極體理想因子
    double I0      = 1e-10;    // 暗電流 (A)

    // 物理常數
    static constexpr double q     = 1.602e-19;  // 電子電荷 (C)
    static constexpr double k_B   = 1.381e-23;  // 波茲曼常數 (J/K)
    static constexpr double G_ref = 1000.0;     // 標準日照 (W/m²)
    static constexpr double T_ref = 298.15;     // 標準溫度 (K) = 25°C
    static constexpr double A_cell= 0.01;       // 電池面積 (m²)

    // 計算熱電壓 Vt = n*k*T/q
    double thermalVoltage(double T_K) const;

    // 計算光生電流 Iph
    double photocurrent(double G, double T_K) const;

    // 牛頓法迭代：給定 V、G(W/m²)、T(°C)，求 I(A)
    double calcCurrent(double V, double G, double T_C) const;

    // 計算開路電壓 Voc（二分法）
    double calcVoc(double G, double T_C) const;
};

#endif
