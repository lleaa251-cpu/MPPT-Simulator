#include "solar_cell.h"
#include <cmath>
#include <stdexcept>

// ============================================================
//  solar_cell.cpp — 單二極體模型實作
// ============================================================

double SolarCell::thermalVoltage(double T_K) const {
    return n * k_B * T_K / q;
}

double SolarCell::photocurrent(double G, double T_K) const {
    double dT = T_K - T_ref;
    return Isc_ref * (G / G_ref) * (1.0 + Ki * dT);
}

// ----------------------------------------------------------
//  牛頓法迭代求解隱式方程式：
//
//  f(I) = I - Iph + I0*(exp((V + I*Rs)/Vt) - 1)
//              + (V + I*Rs)/Rsh = 0
//
//  迭代公式：I_new = I_old - f(I_old) / f'(I_old)
//
//  f'(I) = 1 + (I0*Rs/Vt)*exp((V + I*Rs)/Vt) + Rs/Rsh
// ----------------------------------------------------------
double SolarCell::calcCurrent(double V, double G, double T_C) const {
    double T_K = T_C + 273.15;
    double Vt  = thermalVoltage(T_K);
    double Iph = photocurrent(G, T_K);

    // 初始猜測值（光生電流作起點）
    double I = Iph;

    for (int iter = 0; iter < 100; ++iter) {
        double arg = (V + I * Rs) / Vt;
        // 防止指數溢位
        if (arg > 80.0) arg = 80.0;

        double ex  = std::exp(arg);
        double f   = I - Iph + I0 * (ex - 1.0) + (V + I * Rs) / Rsh;
        double df  = 1.0 + (I0 * Rs / Vt) * ex + Rs / Rsh;

        double dI  = f / df;
        I -= dI;

        if (std::fabs(dI) < 1e-10) break;
    }

    // 電流不可為負（物理限制）
    return (I < 0.0) ? 0.0 : I;
}

// ----------------------------------------------------------
//  二分法求 Voc：找使 I=0 的電壓
//  (牛頓法在靠近 Voc 時可能震盪，二分法較穩定)
// ----------------------------------------------------------
double SolarCell::calcVoc(double G, double T_C) const {
    // Voc 上界：以 Voc_ref 為基準，依溫度線性調整
    // 溫度每升 1°C，Voc 約下降 Kv V
    double Voc_approx = Voc_ref + Kv * (T_C - 25.0);
    // 日照低時 Voc 略降（對數關係），以比例因子近似
    if (G < G_ref) {
        Voc_approx *= (1.0 + 0.04 * std::log(G / G_ref));
    }
    Voc_approx = std::max(1.0, Voc_approx);

    double lo = 0.0, hi = Voc_approx * 1.1;
    for (int i = 0; i < 80; ++i) {
        double mid = (lo + hi) / 2.0;
        if (calcCurrent(mid, G, T_C) > 0.0)
            lo = mid;
        else
            hi = mid;
    }
    return (lo + hi) / 2.0;
}
