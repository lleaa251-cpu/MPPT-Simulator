#include "iv_curve.h"
#include <cmath>

// ============================================================
//  iv_curve.cpp — I-V 曲線掃描實作
// ============================================================

std::vector<DataPoint> scanIVCurve(
    const SolarCell& cell,
    double G,
    double T_C,
    int steps)
{
    std::vector<DataPoint> curve;
    curve.reserve(steps + 1);

    double Voc = cell.calcVoc(G, T_C);

    for (int i = 0; i <= steps; ++i) {
        double V = Voc * static_cast<double>(i) / steps;
        double I = cell.calcCurrent(V, G, T_C);
        double P = V * I;
        curve.push_back({V, I, P});
    }

    return curve;
}
