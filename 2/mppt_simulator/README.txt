================================================================
  MPPT 太陽能板最大功率追蹤模擬器
  Solar Panel Maximum Power Point Tracking Simulator
================================================================

【檔案結構】

  mppt_simulator/
  ├── main.cpp              主程式（選單、流程）
  ├── solar_cell.h/.cpp     單二極體電池模型（牛頓法迭代）
  ├── iv_curve.h/.cpp       I-V 曲線掃描
  ├── mppt.h/.cpp           最大功率點追蹤
  ├── weather_scenario.h/.cpp  預設場景（台灣/北歐）
  ├── output.h/.cpp         終端機輸出 + CSV 匯出
  ├── Makefile              編譯設定
  └── README.txt            本說明檔

【編譯方式 — MinGW / Windows】

  方法一（Makefile）：
    在資料夾內開啟命令提示字元，執行：
      mingw32-make

  方法二（手動 g++）：
    g++ -std=c++17 -O2 -o mppt_simulator ^
        main.cpp solar_cell.cpp iv_curve.cpp ^
        mppt.cpp weather_scenario.cpp output.cpp

【執行】

  mppt_simulator.exe

【核心公式說明】

  1. 單二極體模型：
     I = Iph - I0*(exp((V + I*Rs)/Vt) - 1) - (V + I*Rs)/Rsh

  2. 光生電流：
     Iph = Isc_ref * (G/G_ref) * (1 + Ki*(T_K - T_ref))

  3. 熱電壓：
     Vt = n * k_B * T_K / q

  4. 牛頓法迭代求解 I（隱式方程式）：
     I_new = I_old - f(I_old) / f'(I_old)

  5. 填充因子：
     FF = Pmpp / (Voc * Isc)

  6. 轉換效率：
     eta = Pmpp / (G * A_cell) * 100 (%)

【輸出】

  - 終端機：MPPT 結果表格 + ASCII P-V 折線圖
  - CSV 檔：mppt_result_N.csv（可用 Excel 開啟繪圖）
================================================================
