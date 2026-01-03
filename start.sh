#!/bin/bash
# 快速启动脚本

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}   大文件传输系统 - 快速启动脚本${NC}"
echo -e "${GREEN}========================================${NC}\n"

# 检查Python版本
echo -e "${YELLOW}[1/5] 检查Python环境...${NC}"
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}错误: 未找到Python3,请先安装Python 3.8+${NC}"
    exit 1
fi

PYTHON_VERSION=$(python3 --version | awk '{print $2}')
echo -e "${GREEN}✓ Python版本: $PYTHON_VERSION${NC}\n"

# 检查pip
echo -e "${YELLOW}[2/5] 检查pip...${NC}"
if ! command -v pip3 &> /dev/null; then
    echo -e "${RED}错误: 未找到pip3${NC}"
    exit 1
fi
echo -e "${GREEN}✓ pip3 已安装${NC}\n"

# 安装依赖
echo -e "${YELLOW}[3/5] 安装Python依赖...${NC}"
pip3 install -r requirements.txt -q
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ 依赖安装成功${NC}\n"
else
    echo -e "${RED}错误: 依赖安装失败${NC}"
    exit 1
fi

# 创建必要的目录
echo -e "${YELLOW}[4/5] 创建数据目录...${NC}"
mkdir -p /var/uploads
mkdir -p /var/uploads/temp
mkdir -p /var/downloads
echo -e "${GREEN}✓ 目录创建完成${NC}\n"

# 启动服务
echo -e "${YELLOW}[5/5] 启动服务...${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}服务启动成功!${NC}"
echo -e "${GREEN}访问地址: http://0.0.0.0:5000${NC}"
echo -e "${GREEN}========================================${NC}\n"

# 后台运行
nohup python3 app.py > server.log 2>&1 &

# 保存PID
echo $! > server.pid

echo -e "${GREEN}✓ 服务已在后台启动${NC}"
echo -e "${YELLOW}日志文件: server.log${NC}"
echo -e "${YELLOW}PID文件: server.pid${NC}\n"

echo -e "查看日志: ${GREEN}tail -f server.log${NC}"
echo -e "停止服务: ${GREEN}kill \$(cat server.pid)${NC}"
echo -e "重启服务: ${GREEN}./restart.sh${NC}\n"
