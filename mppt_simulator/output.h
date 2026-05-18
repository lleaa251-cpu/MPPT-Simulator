#ifndef OUTPUT_H
#define OUTPUT_H

// ============================================================
//  output.h — 輸出模組（終端機 + CSV）
// ============================================================

#include "mppt.h"
#include "iv_curve.h"
#include <string>
#include <vector>

// 印出分隔線
void printSeparator(char c = '=', int width = 56);

// 印出 MPPT 結果表格
void printMPPTResult(const MPPTResult& r, double G, double T_C,
                     const std::string& sceneName);

// 印出 ASCII P-V 曲線（終端機折線圖）
// chartWidth：橫軸字元數，chartHeight：縱軸列數
void printPVChart(const std::vector<DataPoint>& curve,
                  const MPPTResult& r,
                  int chartWidth = 50,
                  int chartHeight = 16);

// 輸出 CSV 檔案（voltage, current, power 三欄）
// 回傳 true 表示成功
bool exportCSV(const std::vector<DataPoint>& curve,
               const MPPTResult& r,
               double G, double T_C,
               const std::string& filename);

#endif
