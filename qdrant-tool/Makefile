# Qdrant Tool - Standalone Makefile
# 完全独立的 Makefile，无需 CMake

# 项目配置
PROJECT_NAME := qdrant-tool
VERSION := 1.0.0

# 编译器设置
CXX := g++
CXX_STANDARD := c++17

# 编译选项：默认 -O2，无调试信息
CXXFLAGS := -std=$(CXX_STANDARD) -O2 -Wall -Wextra -MMD -MP -fPIC

# 包含目录
INCLUDES := -I./include -I./include/third_party

# 链接策略：全部动态链接
# 系统基础库（glibc、openssl、pthread、zlib、curl 等）
LDFLAGS := -lcurl -lssl -lcrypto -lz -lpthread -ldl

# 源文件目录
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)

# 源文件列表
SOURCES := \
    $(SRC_DIR)/main.cpp \
    $(SRC_DIR)/infrastructure/http_client.cpp \
    $(SRC_DIR)/infrastructure/config_manager.cpp \
    $(SRC_DIR)/adapter/qdrant_client.cpp \
    $(SRC_DIR)/adapter/ollama_client.cpp \
    $(SRC_DIR)/service/memory_service.cpp \
    $(SRC_DIR)/service/collection_service.cpp \
    $(SRC_DIR)/cli/argument_parser.cpp \
    $(SRC_DIR)/cli/command_router.cpp \
    $(SRC_DIR)/utils/uuid.cpp \
    $(SRC_DIR)/utils/file_reader.cpp \
    $(SRC_DIR)/utils/text_chunker.cpp

# 目标文件
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

# 依赖文件
DEPS := $(OBJECTS:.o=.d)

# 可执行文件
TARGET := $(BIN_DIR)/$(PROJECT_NAME)

# 默认目标
.PHONY: all clean install uninstall help check-libs

all: $(TARGET)
	@echo ""
	@echo "Build complete: $(TARGET)"
	@echo ""
	@echo "All libraries are dynamically linked:"
	@ldd $(TARGET) | grep -E "(curl|ssl|crypto|z|pthread)" || true

# 创建构建目录
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/infrastructure
	@mkdir -p $(BUILD_DIR)/adapter
	@mkdir -p $(BUILD_DIR)/service
	@mkdir -p $(BUILD_DIR)/cli
	@mkdir -p $(BUILD_DIR)/utils

# 编译规则
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 链接规则
$(TARGET): $(OBJECTS)
	@echo "Linking $(PROJECT_NAME)..."
	@echo "  Libraries: libcurl, libssl, libcrypto, zlib, libpthread (all dynamic)"
	@$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# 包含依赖文件
-include $(DEPS)

# 清理
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete."

# 检查链接情况
check-libs: $(TARGET)
	@echo "Checking $(TARGET) dynamic dependencies..."
	@file $(TARGET)
	@echo ""
	@ldd $(TARGET)

# 安装
PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin

install: $(TARGET)
	@echo "Installing $(PROJECT_NAME) to $(BINDIR)..."
	@install -d $(BINDIR)
	@install -m 755 $(TARGET) $(BINDIR)/$(PROJECT_NAME)
	@echo "Installation complete."

uninstall:
	@echo "Uninstalling $(PROJECT_NAME)..."
	@rm -f $(BINDIR)/$(PROJECT_NAME)
	@echo "Uninstall complete."

# 运行
run: $(TARGET)
	@./$(TARGET)

# 帮助
help:
	@echo "Qdrant Tool - Build System"
	@echo ""
	@echo "Usage: make [target] [options]"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build the project (default)"
	@echo "  clean       - Remove build files"
	@echo "  check-libs  - Check binary dynamic dependencies"
	@echo "  install     - Install to system (default: /usr/local/bin)"
	@echo "  uninstall   - Remove from system"
	@echo "  run         - Build and run"
	@echo "  help        - Show this help"
	@echo ""
	@echo "Options:"
	@echo "  PREFIX=path  - Set installation prefix"
	@echo "  CXX=compiler - Use different compiler (default: g++)"
	@echo ""
	@echo "Build Configuration:"
	@echo "  Optimization: -O2 (no debug info)"
	@echo "  Linking:      All dynamic (libcurl, libssl, libcrypto, zlib, libpthread)"
	@echo ""
	@echo "Examples:"
	@echo "  make                       # Build the project"
	@echo "  make check-libs            # Check dynamic dependencies"
	@echo "  make install PREFIX=~/.local  # Install to ~/.local/bin"

# 防止删除中间文件
.PRECIOUS: $(BUILD_DIR)/%.o
