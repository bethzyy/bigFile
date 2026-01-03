#!/bin/bash
# 停止服务脚本

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}正在停止服务...${NC}"

if [ -f server.pid ]; then
    PID=$(cat server.pid)
    if ps -p $PID > /dev/null 2>&1; then
        kill $PID
        rm server.pid
        echo -e "${GREEN}✓ 服务已停止 (PID: $PID)${NC}"
    else
        echo -e "${RED}进程不存在 (PID: $PID)${NC}"
        rm server.pid
    fi
else
    echo -e "${YELLOW}未找到PID文件,尝试查找进程...${NC}"
    PIDS=$(ps aux | grep 'python3 app.py' | grep -v grep | awk '{print $2}')
    if [ -n "$PIDS" ]; then
        echo $PIDS | xargs kill
        echo -e "${GREEN}✓ 已停止相关进程${NC}"
    else
        echo -e "${RED}未找到运行中的服务${NC}"
    fi
fi
