#!/bin/bash
# Systemd 服务安装脚本 (C语言版本)

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# 检查是否为root用户
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}错误: 请使用 sudo 运行此脚本${NC}"
    exit 1
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  安装 Systemd 服务${NC}"
echo -e "${GREEN}  (C语言版本)${NC}"
echo -e "${GREEN}========================================${NC}\n"

# 获取当前目录
SERVICE_DIR=$(pwd)
SERVICE_NAME="filetransfer"
EXECUTABLE="$SERVICE_DIR/fileserver"

# 检查可执行文件
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}错误: 未找到 fileserver 可执行文件${NC}"
    echo -e "${YELLOW}请先运行: make${NC}"
    exit 1
fi

echo -e "${YELLOW}创建服务文件...${NC}"

cat > /etc/systemd/system/${SERVICE_NAME}.service << EOF
[Unit]
Description=Large File Transfer Server (C Version)
After=network.target

[Service]
Type=simple
User=${SUDO_USER:-root}
WorkingDirectory=${SERVICE_DIR}
ExecStart=${EXECUTABLE}
Restart=always
RestartSec=5

# 日志
StandardOutput=journal
StandardError=journal

# 安全设置
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=${SERVICE_DIR} /var/uploads /var/downloads

# 资源限制
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
EOF

echo -e "${GREEN}✓ 服务文件创建完成${NC}\n"

# 重载systemd
echo -e "${YELLOW}重载 systemd 配置...${NC}"
systemctl daemon-reload
echo -e "${GREEN}✓ 配置已重载${NC}\n"

# 启用服务
echo -e "${YELLOW}启用开机自启动...${NC}"
systemctl enable ${SERVICE_NAME}
echo -e "${GREEN}✓ 开机自启动已启用${NC}\n"

# 启动服务
echo -e "${YELLOW}启动服务...${NC}"
systemctl start ${SERVICE_NAME}

# 等待服务启动
sleep 2

echo -e "${GREEN}✓ 服务已启动${NC}\n"

# 显示状态
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}服务安装成功!${NC}"
echo -e "${GREEN}========================================${NC}\n"

echo -e "服务状态: ${YELLOW}systemctl status ${SERVICE_NAME}${NC}"
echo -e "查看日志: ${YELLOW}journalctl -u ${SERVICE_NAME} -f${NC}"
echo -e "启动服务: ${YELLOW}systemctl start ${SERVICE_NAME}${NC}"
echo -e "停止服务: ${YELLOW}systemctl stop ${SERVICE_NAME}${NC}"
echo -e "重启服务: ${YELLOW}systemctl restart ${SERVICE_NAME}${NC}"
echo -e "禁用自启: ${YELLOW}systemctl disable ${SERVICE_NAME}${NC}\n"

# 显示当前状态
systemctl status ${SERVICE_NAME} --no-pager
