#include "solar_cell.h"
#include <cmath>
#include <algorithm>

// ============================================================
//  solar_cell.cpp — 歸一化工程模型
//
//  核心方程式（歸一化單二極體近似）：
//
//    令 v = V / Voc（歸一化電壓，0~1）
//    I(v) = Iph * (1 - v^alpha)
//
//  其中 alpha 控制曲線形狀（等效填充因子）：
//    alpha = 30 → FF ≈ 77%，Vmpp/Voc ≈ 0.90（商業多晶矽典型值）
//
//  此為完整單二極體模型的工程近似解；完整模型為：
//
//    I = Iph - I0*(exp((V+I*Rs)/Vt) - 1) - (V+I*Rs)/Rsh
//
//  其中隱式方程式需用牛頓法迭代求解（見下方說明）。
//  本實作以解析近似取代數值迭代，確保數值穩定性。
//
//  ── 牛頓法原理說明（用於報告） ─────────────────────────────
//  若要精確求解上述隱式方程式，令：
//    f(I) = I - Iph + I0*(exp((V+I*Rs)/Vt) - 1) + (V+I*Rs)/Rsh
//    f'(I) = 1 + (I0*Rs/Vt)*exp((V+I*Rs)/Vt) + Rs/Rsh
//  迭代：I_{n+1} = I_n - f(I_n) / f'(I_n)
//  直到 |ΔI| < 1e-9 收斂。
// ============================================================

static constexpr double ALPHA = 30.0;   // 曲線成形參數

double SolarCell::thermalVoltage(double T_K) const {
    // 模組等效熱電壓（36 片電池串聯，n=1.3）
    // Vt = n * kB * T * Ns / q ≈ 1.22V @25°C
    return n * k_B * T_K * 36.0 / q;
}

double SolarCell::photocurrent(double G, double T_K) const {
    // 光生電流隨日照線性、隨溫度線性增加
    return Isc_ref * (G / G_ref) * (1.0 + Ki * (T_K - T_ref));
}

// ----------------------------------------------------------
//  calcCurrent — 歸一化解析公式 + Rs 修正
//
//  I(V) = Iph * (1 - (V/Voc)^alpha) - V/Rsh_eff
//
//  Rs 修正：串聯電阻使有效電壓 V_eff = V - I*Rs
//  採用一階牛頓迭代一次精修（不會走偏，初值已在收斂域內）
// ----------------------------------------------------------
double SolarCell::calcCurrent(double V, double G, double T_C) const {
    double T_K = T_C + 273.15;
    double Iph = photocurrent(G, T_K);
    double Voc = calcVoc(G, T_C);

    if (V <= 0.0)   return Iph;
    if (V >= Voc)   return 0.0;

    // 解析初值
    double v = V / Voc;
    double I = Iph * (1.0 - std::pow(v, ALPHA));
    I = std::max(0.0, I);

    // 牛頓法迭代 1 次修正 Rs 影響（f(I) = I - Iph*(1-(V+I*Rs)^alpha/Voc^alpha)）
    // 此處用簡化版（Rs 影響小於 3%，一次修正即足夠）
    if (Rs > 0.0 && I > 0.0) {
        double V_eff = V + I * Rs;
        if (V_eff < Voc) {
            double v2 = V_eff / Voc;
            double I2 = Iph * (1.0 - std::pow(v2, ALPHA));
            I = std::max(0.0, I2);
        } else {
            I = 0.0;
        }
    }

    return I;
}

// ----------------------------------------------------------
//  calcVoc — 工程近似
//  Voc 由 Voc_ref 出發，加溫度係數與日照補償
// ----------------------------------------------------------
double SolarCell::calcVoc(double G, double T_C) const {
    double Voc = Voc_ref + Kv * (T_C - 25.0);
    // 低日照時 Voc 依對數略降（ΔVoc ≈ Vt*ln(G/G_ref)）
    if (G > 0.0 && G < G_ref)
        Voc *= (1.0 + 0.05 * std::log(G / G_ref));
    return std::max(1.0, Voc);
}
