#!/bin/bash
# C语言版本快速启动脚本

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  大文件传输服务器 - 启动脚本${NC}"
echo -e "${GREEN}  (C语言高性能版本)${NC}"
echo -e "${GREEN}========================================${NC}\n"

# 检查GCC编译器
echo -e "${YELLOW}[1/6] 检查编译环境...${NC}"
if ! command -v gcc &> /dev/null; then
    echo -e "${RED}错误: 未找到GCC编译器${NC}"
    echo -e "${YELLOW}请安装: sudo apt install build-essential${NC}"
    exit 1
fi

GCC_VERSION=$(gcc --version | head -n1)
echo -e "${GREEN}✓ $GCC_VERSION${NC}\n"

# 检查libmicrohttpd
echo -e "${YELLOW}[2/6] 检查libmicrohttpd库...${NC}"
if ! pkg-config --exists libmicrohttpd; then
    echo -e "${RED}错误: 未找到libmicrohttpd开发库${NC}"
    echo -e "${YELLOW}请安装: sudo apt install libmicrohttpd-dev${NC}"
    exit 1
fi

MHD_VERSION=$(pkg-config --modversion libmicrohttpd)
echo -e "${GREEN}✓ libmicrohttpd 版本: $MHD_VERSION${NC}\n"

# 检查源文件
echo -e "${YELLOW}[3/6] 检查源文件...${NC}"
if [ ! -f "server.c" ]; then
    echo -e "${RED}错误: 未找到 server.c 源文件${NC}"
    exit 1
fi
echo -e "${GREEN}✓ 源文件检查通过${NC}\n"

# 编译程序
echo -e "${YELLOW}[4/6] 编译程序...${NC}"
if [ ! -f "fileserver" ]; then
    make
    if [ $? -ne 0 ]; then
        echo -e "${RED}错误: 编译失败${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ 编译成功${NC}\n"
else
    echo -e "${GREEN}✓ 可执行文件已存在${NC}\n"
fi

# 创建必要的目录
echo -e "${YELLOW}[5/6] 创建数据目录...${NC}"
mkdir -p /var/uploads
mkdir -p /var/uploads/temp
mkdir -p /var/downloads
echo -e "${GREEN}✓ 目录创建完成${NC}\n"

# 启动服务
echo -e "${YELLOW}[6/6] 启动服务...${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}服务启动成功!${NC}"
echo -e "${GREEN}访问地址: http://0.0.0.0:5000${NC}"
echo -e "${GREEN}========================================${NC}\n"

# 检查端口是否被占用
if command -v lsof &> /dev/null; then
    PORT_PID=$(lsof -ti:5000)
    if [ -n "$PORT_PID" ]; then
        echo -e "${YELLOW}警告: 端口5000已被占用 (PID: $PORT_PID)${NC}"
        echo -e "${YELLOW}尝试终止占用进程...${NC}"
        kill -9 $PORT_PID 2>/dev/null
        sleep 1
    fi
fi

# 后台运行
nohup ./fileserver > server.log 2>&1 &

# 保存PID
echo $! > server.pid

echo -e "${GREEN}✓ 服务已在后台启动${NC}"
echo -e "${YELLOW}日志文件: server.log${NC}"
echo -e "${YELLOW}PID文件: server.pid${NC}\n"

echo -e "查看日志: ${GREEN}tail -f server.log${NC}"
echo -e "停止服务: ${GREEN}./stop.sh${NC}"
echo -e "重启服务: ${GREEN}./restart.sh${NC}"
echo -e "查看状态: ${GREEN}ps aux | grep fileserver${NC}\n"
