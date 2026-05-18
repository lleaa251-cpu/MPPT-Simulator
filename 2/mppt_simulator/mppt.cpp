#include "mppt.h"
#include <algorithm>

// ============================================================
//  mppt.cpp — 最大功率點追蹤實作
// ============================================================

MPPTResult findMPP(
    const std::vector<DataPoint>& curve,
    double G,
    double A_cell)
{
    // 找功率最大的資料點（線性掃描）
    auto mppIt = std::max_element(
        curve.begin(), curve.end(),
        [](const DataPoint& a, const DataPoint& b) {
            return a.P < b.P;
        }
    );

    double Voc = curve.back().V;   // Voc ≈ 最後一點的電壓
    double Isc = curve.front().I;  // Isc ≈ V=0 時的電流

    double Pmpp = mppIt->P;
    double FF   = (Voc > 0 && Isc > 0) ? Pmpp / (Voc * Isc) : 0.0;
    double eta  = (G > 0 && A_cell > 0) ? (Pmpp / (G * A_cell)) * 100.0 : 0.0;

    return {
        mppIt->V,
        mppIt->I,
        Pmpp,
        Voc,
        Isc,
        FF,
        eta
    };
}
