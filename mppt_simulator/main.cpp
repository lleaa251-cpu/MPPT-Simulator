#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <cstdlib>

#include "solar_cell.h"
#include "iv_curve.h"
#include "mppt.h"
#include "weather_scenario.h"
#include "output.h"

// ============================================================
//  main.cpp — MPPT 太陽能板最大功率追蹤模擬器
//
//  作者：郭青彤
//  說明：單二極體模型 + 牛頓法迭代 + ASCII 輸出 + CSV 匯出
// ============================================================

// 清除輸入緩衝區
void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// 安全讀取 double（含範圍檢查）
double readDouble(const std::string& prompt, double lo, double hi) {
    double val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && val >= lo && val <= hi) {
            clearInputBuffer();
            return val;
        }
        std::cout << "  [!] 請輸入 " << lo << " ~ " << hi << " 之間的數值。\n";
        clearInputBuffer();
    }
}

// 輸出主選單
void printMainMenu(const std::vector<Scenario>& scenes) {
    printSeparator('=');
    std::cout << "       MPPT 太陽能板最大功率追蹤模擬器\n";
    printSeparator('=');
    std::cout << "  請選擇模擬場景：\n\n";
    for (size_t i = 0; i < scenes.size(); ++i) {
        std::cout << "  " << scenes[i].name;
        if (i < scenes.size() - 1) {
            std::cout << "  (G=" << scenes[i].G
                      << " W/m2, T=" << scenes[i].T_C << " C)";
        }
        std::cout << "\n";
    }
    std::cout << "  [0] 結束程式\n";
    printSeparator('-');
    std::cout << "  輸入選項：";
}

int main() {
    // 初始化太陽能電池（使用預設參數）
    SolarCell cell;

    // 取得預設場景清單
    auto scenes = getPresetScenarios();
    const int CUSTOM_IDX = static_cast<int>(scenes.size()) - 1;  // 自訂場景索引

    int runCount = 0;

    while (true) {
        std::cout << "\n";
        printMainMenu(scenes);

        int choice;
        if (!(std::cin >> choice)) {
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        if (choice == 0) {
            std::cout << "\n  感謝使用 MPPT 模擬器。再見！\n\n";
            break;
        }

        // 驗證選項範圍
        if (choice < 1 || choice > static_cast<int>(scenes.size())) {
            std::cout << "  [!] 無效選項，請重新輸入。\n";
            continue;
        }

        double G, T_C;
        std::string sceneName;

        if (choice - 1 == CUSTOM_IDX) {
            // 自訂場景：請使用者輸入
            std::cout << "\n";
            G      = readDouble("  請輸入日照強度 G (W/m2) [50 ~ 1200]: ", 50.0, 1200.0);
            T_C    = readDouble("  請輸入環境溫度 T (°C)  [0  ~ 70  ]: ", 0.0, 70.0);
            sceneName = "自訂條件";
        } else {
            G         = scenes[choice - 1].G;
            T_C       = scenes[choice - 1].T_C;
            sceneName = scenes[choice - 1].name;
            // 去掉前綴 "[n] " 只保留說明文字
            if (sceneName.size() > 4) sceneName = sceneName.substr(4);
        }

        std::cout << "\n  [計算中...]\n";

        // Step 1：掃描 I-V 曲線（300 點以提高 MPP 精度）
        auto curve = scanIVCurve(cell, G, T_C, 300);

        // Step 2：找最大功率點
        MPPTResult result = findMPP(curve, G, cell.A_cell);

        // Step 3：輸出結果表格
        std::cout << "\n";
        printMPPTResult(result, G, T_C, sceneName);

        // Step 4：輸出 ASCII P-V 圖
        printPVChart(curve, result);

        // Step 5：詢問是否匯出 CSV
        std::cout << "  是否匯出 CSV 檔案？(y/n): ";
        char ans;
        std::cin >> ans;
        clearInputBuffer();

        if (ans == 'y' || ans == 'Y') {
            ++runCount;
            std::string filename = "mppt_result_" + std::to_string(runCount) + ".csv";
            bool ok = exportCSV(curve, result, G, T_C, filename);
            if (ok)
                std::cout << "  [OK] 已儲存：" << filename << "\n";
            else
                std::cout << "  [!!] 檔案寫入失敗，請確認資料夾寫入權限。\n";
        }

        // Step 6：多場景比較提示
        std::cout << "\n  提示：可繼續選擇其他場景進行比較。\n";
    }

    return 0;
}
