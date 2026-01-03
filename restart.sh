#!/bin/bash
# 重启服务脚本

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}正在重启服务...${NC}\n"

# 停止服务
./stop.sh

# 等待2秒
sleep 2

# 启动服务
./start.sh
