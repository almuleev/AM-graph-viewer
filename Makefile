# Build the LVM Reader CLI with g++ (MSYS2/MinGW or any C++17 compiler).
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -finput-charset=UTF-8
WINDRES  ?= windres
# Static linking keeps the binary self-contained (no libstdc++/libgcc DLLs).
LDFLAGS  ?= -static
TARGET   := lvm_reader
# Identify the actual checkout, including uncommitted source changes.
VERSION  := $(shell git describe --tags --always --dirty 2>/dev/null || echo dev)
CPPFLAGS += -DAPP_VERSION=\"$(VERSION)\"

# Parser/analysis library shared by the CLI and the tests.
LIB_SRC  := lvm_parser.cpp fft.cpp analysis.cpp data_io.cpp filter_engine.cpp spectrum_worker.cpp
APP_SRC  := main.cpp $(LIB_SRC)
APP_OBJ  := $(APP_SRC:.cpp=.o)
HDRS     := $(wildcard *.hpp)
GUI_PARTS := $(wildcard gui_*.cpp)
GUI_RES  := AM_logo.o

ifeq ($(OS),Windows_NT)
    BIN      := $(TARGET).exe
    TEST_BIN := tests/run_tests.exe
    GUI_BIN  := AMGraphViewer-$(VERSION)-win-x64.exe
else
    BIN      := $(TARGET)
    TEST_BIN := tests/run_tests
    GUI_BIN  := AMGraphViewer-$(VERSION)
endif

.PHONY: all clean run test gui test-gui FORCE

all: $(BIN)

# Refresh the CLI version even when only the checkout/dirty state changed.
main.o: FORCE
FORCE:

$(BIN): $(APP_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(APP_OBJ) $(LDFLAGS)

%.o: %.cpp $(HDRS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN) lvm_files_for_tests/test.lvm

test: $(TEST_BIN)
	./$(TEST_BIN)

test-gui: tests/gui_regression.exe
	./tests/gui_regression.exe

tests/gui_regression.exe: tests/gui_regression.cpp $(GUI_PARTS) $(LIB_SRC) export_helpers.cpp formula_engine.cpp gap_details.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -I. -o $@ tests/gui_regression.cpp $(LIB_SRC) export_helpers.cpp formula_engine.cpp gap_details.cpp $(LDFLAGS) -lcomdlg32 -lgdi32 -luser32 -lgdiplus -lcomctl32

$(TEST_BIN): tests/run_tests.cpp $(LIB_SRC) export_helpers.cpp formula_engine.cpp gap_details.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -I. -o $@ tests/run_tests.cpp $(LIB_SRC) export_helpers.cpp formula_engine.cpp gap_details.cpp $(LDFLAGS)

# Native Win32 GUI viewer (Windows only). Needs -municode for wWinMain and the
# Win32 import libraries. On Windows you can also run: powershell ./build_gui.ps1
gui: $(GUI_BIN)

$(GUI_RES): AM_logo.rc AM_logo.ico
	$(WINDRES) -O coff -i $< -o $@

$(GUI_BIN): $(GUI_PARTS) gap_details.cpp export_helpers.cpp formula_engine.cpp $(LIB_SRC) $(HDRS) $(GUI_RES)
	$(CXX) $(CXXFLAGS) -DAPP_VERSION_W=L\"$(VERSION)\" -municode -mwindows -o $@ gui_main.cpp gap_details.cpp $(LIB_SRC) export_helpers.cpp formula_engine.cpp $(GUI_RES) $(LDFLAGS) -lcomdlg32 -lgdi32 -luser32 -lgdiplus -lcomctl32

clean:
	rm -f $(APP_OBJ) $(BIN) $(TEST_BIN) $(GUI_BIN) $(GUI_RES)
