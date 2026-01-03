#!/bin/bash
# 一键编译和安装脚本

echo "=========================================="
echo "  大文件传输服务器 - 编译安装脚本"
echo "  (C语言版本)"
echo "=========================================="
echo ""

# 检查依赖
echo "[1/3] 检查依赖..."
if ! command -v gcc &> /dev/null; then
    echo "错误: 未找到GCC"
    echo "请安装: sudo apt install build-essential"
    exit 1
fi

if ! pkg-config --exists libmicrohttpd; then
    echo "错误: 未找到libmicrohttpd"
    echo "请安装: sudo apt install libmicrohttpd-dev"
    exit 1
fi

echo "✓ 依赖检查通过"
echo ""

# 编译
echo "[2/3] 编译程序..."
make clean
make

if [ $? -ne 0 ]; then
    echo "错误: 编译失败"
    exit 1
fi

echo "✓ 编译成功"
echo ""

# 创建目录
echo "[3/3] 创建数据目录..."
mkdir -p /var/uploads
mkdir -p /var/uploads/temp
mkdir -p /var/downloads
echo "✓ 目录创建完成"
echo ""

echo "=========================================="
echo "  安装完成!"
echo "=========================================="
echo ""
echo "运行服务:"
echo "  ./fileserver"
echo ""
echo "或使用启动脚本:"
echo "  ./start_c.sh"
echo ""
echo "安装为系统服务:"
echo "  sudo ./install_service_c.sh"
echo ""
