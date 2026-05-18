# ============================================================
#  Makefile — MPPT 模擬器（MinGW g++ / Windows）
#  使用方式：在程式碼資料夾內執行 mingw32-make
# ============================================================

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

TARGET   = mppt_simulator
SRCS     = main.cpp solar_cell.cpp iv_curve.cpp mppt.cpp \
           weather_scenario.cpp output.cpp
OBJS     = $(SRCS:.cpp=.o)

# 預設目標：編譯
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)
	@echo "==> 編譯完成：$(TARGET).exe"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清除編譯產物
clean:
	del /Q *.o $(TARGET).exe 2>nul || true

.PHONY: all clean
