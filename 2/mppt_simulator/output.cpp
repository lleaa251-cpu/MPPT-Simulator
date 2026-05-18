#include "output.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

// ============================================================
//  output.cpp — 輸出模組實作
// ============================================================

void printSeparator(char c, int width) {
    std::cout << std::string(width, c) << "\n";
}

// ----------------------------------------------------------
//  printMPPTResult — 輸出 MPPT 結果表格
// ----------------------------------------------------------
void printMPPTResult(const MPPTResult& r, double G, double T_C,
                     const std::string& sceneName)
{
    printSeparator('=');
    std::cout << "  MPPT 模擬結果\n";
    printSeparator('=');

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  場景     : " << sceneName               << "\n";
    std::cout << "  日照強度 : " << G    << " W/m2\n";
    std::cout << "  溫度     : " << T_C  << " deg C\n";
    printSeparator('-');
    std::cout << "  開路電壓  Voc  = " << std::setw(7) << r.Voc   << " V\n";
    std::cout << "  短路電流  Isc  = " << std::setw(7) << r.Isc   << " A\n";
    printSeparator('-');
    std::cout << "  [MPP] 最大功率點電壓 Vmpp = " << std::setw(7) << r.Vmpp << " V\n";
    std::cout << "  [MPP] 最大功率點電流 Impp = " << std::setw(7) << r.Impp << " A\n";
    std::cout << "  [MPP] 最大輸出功率   Pmpp = " << std::setw(7) << r.Pmpp << " W\n";
    printSeparator('-');
    std::cout << "  填充因子  FF   = " << std::setw(7) << r.FF * 100.0 << " %\n";
    std::cout << "  轉換效率  eta  = " << std::setw(7) << r.eta         << " %\n";

    // 效率評語
    std::cout << "\n  效率評估：";
    if (r.eta >= 15.0)
        std::cout << "優良（商業級多晶矽水準）";
    else if (r.eta >= 10.0)
        std::cout << "普通（可接受）";
    else
        std::cout << "偏低（低日照或高溫影響顯著）";
    std::cout << "\n";
    printSeparator('=');
}

// ----------------------------------------------------------
//  printPVChart — ASCII P-V 折線圖
//
//  縱軸：功率 P（W）
//  橫軸：電壓 V（V）
//  MPP 用 [*] 標記
// ----------------------------------------------------------
void printPVChart(const std::vector<DataPoint>& curve,
                  const MPPTResult& r,
                  int W, int H)
{
    if (curve.empty()) return;

    double Voc  = curve.back().V;
    double Pmax = r.Pmpp * 1.15;   // 縱軸上界留 15% 空間

    // 建立字元網格（空格初始化）
    std::vector<std::string> grid(H, std::string(W, ' '));

    // 將曲線資料點投影到網格
    for (const auto& dp : curve) {
        if (Voc <= 0 || Pmax <= 0) break;
        int col = static_cast<int>((dp.V / Voc) * (W - 1));
        int row = H - 1 - static_cast<int>((dp.P / Pmax) * (H - 1));
        col = std::max(0, std::min(W - 1, col));
        row = std::max(0, std::min(H - 1, row));
        grid[row][col] = '.';
    }

    // 標記 MPP 位置
    int mppCol = static_cast<int>((r.Vmpp / Voc)  * (W - 1));
    int mppRow = H - 1 - static_cast<int>((r.Pmpp / Pmax) * (H - 1));
    mppCol = std::max(0, std::min(W - 1, mppCol));
    mppRow = std::max(0, std::min(H - 1, mppRow));
    grid[mppRow][mppCol] = '*';

    // 輸出圖表
    std::cout << "\n  P-V 曲線（* = 最大功率點 MPP）\n\n";
    std::cout << std::fixed << std::setprecision(1);

    for (int r_i = 0; r_i < H; ++r_i) {
        double pLabel = Pmax * (H - 1 - r_i) / (H - 1);
        std::cout << "  " << std::setw(6) << pLabel << "W |";
        std::cout << grid[r_i] << "\n";
    }

    // 橫軸
    std::cout << "         +" << std::string(W, '-') << "\n";

    // 橫軸刻度（5 個刻度）
    std::cout << "          ";
    for (int t = 0; t <= 4; ++t) {
        double vLabel = Voc * t / 4.0;
        std::string s = std::to_string(static_cast<int>(vLabel * 10) / 10);
        std::cout << std::left << std::setw(W / 4 + (t == 0 ? 1 : 2)) << (s + "V");
    }
    std::cout << "\n\n";
}

// ----------------------------------------------------------
//  exportCSV — 輸出 CSV 檔案
// ----------------------------------------------------------
bool exportCSV(const std::vector<DataPoint>& curve,
               const MPPTResult& r,
               double G, double T_C,
               const std::string& filename)
{
    std::ofstream ofs(filename);
    if (!ofs.is_open()) return false;

    // 標頭資訊（Excel 開啟後的備註列）
    ofs << "# MPPT Simulator - 模擬結果匯出\n";
    ofs << "# 日照強度(W/m2)," << G   << "\n";
    ofs << "# 溫度(degC),"     << T_C << "\n";
    ofs << "# Vmpp(V),"  << r.Vmpp << ",Impp(A)," << r.Impp
        << ",Pmpp(W),"   << r.Pmpp << ",FF,"       << r.FF
        << ",eta(%),"    << r.eta  << "\n";
    ofs << "#\n";

    // 欄位標題
    ofs << "Voltage(V),Current(A),Power(W)\n";

    ofs << std::fixed << std::setprecision(6);
    for (const auto& dp : curve) {
        ofs << dp.V << "," << dp.I << "," << dp.P << "\n";
    }

    ofs.close();
    return true;
}
