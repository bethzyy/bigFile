# 大文件传输系统

一个专为Linux服务器设计的高性能大文件上传下载系统,支持 >10GB 文件的稳定传输。

## 📦 项目文件结构

```
hkk/
├── app.py                      # Flask后端服务 (主程序)
├── templates/
│   └── index.html             # 前端页面 (现代化UI)
├── requirements.txt           # Python依赖
├── README.md                  # 详细文档 (本文件)
├── start.sh                   # 快速启动脚本
├── stop.sh                    # 停止服务脚本
├── restart.sh                 # 重启服务脚本
├── install_service.sh         # Systemd服务安装
├── .env.example              # 环境变量示例
└── .gitignore                # Git忽略文件
```

## ✨ 核心特性

1. **大文件支持** - 可处理 10GB+ 文件的无障碍传输
2. **智能分片** - 自动将文件分割为 100MB 分片并行上传
3. **断点续传** - 支持网络中断后继续传输,无需重新开始
4. **实时进度** - 精确显示上传速度、进度百分比和剩余时间
5. **现代化UI** - 响应式设计,支持拖拽上传,界面美观易用
6. **并发处理** - 基于Flask异步处理,支持多用户同时使用
7. **安全可靠** - 文件名安全过滤,防止路径遍历攻击

## 📋 系统要求

- **操作系统**: Linux (Ubuntu 20.04+, CentOS 7+, Debian 10+)
- **Python**: 3.8 或更高版本
- **内存**: 建议 4GB+ (处理超大文件时)
- **磁盘空间**: 根据需要存储的文件大小而定
- **网络**: 建议千兆网络环境

## 🚀 快速部署

### 方法1: 使用启动脚本 (推荐)

最简单的启动方式,自动处理所有配置:

```bash
# 在Linux服务器上执行
cd /path/to/hkk
./start.sh
```

**启动脚本会自动完成:**
- ✓ 检查Python环境和版本
- ✓ 安装所有必要的依赖包
- ✓ 创建必要的数据目录 (/var/uploads, /var/downloads)
- ✓ 启动Flask服务
- ✓ 保存进程PID便于管理

**查看运行状态:**
```bash
# 查看实时日志
tail -f server.log

# 停止服务
./stop.sh

# 重启服务
./restart.sh
```

### 方法2: 安装为系统服务 (生产环境推荐)

创建systemd服务,支持开机自启动和自动重启:

```bash
sudo ./install_service.sh
```

**Systemd服务提供以下功能:**
- ✓ 开机自动启动
- ✓ 服务崩溃自动重启
- ✓ 系统日志集成 (journalctl)
- ✓ 标准化服务管理

**服务管理命令:**
```bash
# 查看服务状态
systemctl status file-transfer

# 启动服务
systemctl start file-transfer

# 停止服务
systemctl stop file-transfer

# 重启服务
systemctl restart file-transfer

# 查看日志
journalctl -u file-transfer -f

# 禁用开机自启
systemctl disable file-transfer

# 启用开机自启
systemctl enable file-transfer
```

### 方法3: 手动部署

如果需要自定义配置,可以手动部署:

```bash
# 1. 安装 Python 3.8+
sudo apt update
sudo apt install python3 python3-pip -y  # Ubuntu/Debian
# 或
sudo yum install python3 python3-pip -y  # CentOS/RHEL

# 2. 安装项目依赖
pip3 install -r requirements.txt

# 3. 创建数据目录
sudo mkdir -p /var/uploads
sudo mkdir -p /var/uploads/temp
sudo mkdir -p /var/downloads
sudo chmod 755 /var/uploads /var/downloads

# 4. 配置环境变量 (可选)
cp .env.example .env
# 编辑 .env 文件自定义配置

# 5. 启动服务
# 方式A: 前台运行 (开发调试)
python3 app.py

# 方式B: 后台运行
nohup python3 app.py > server.log 2>&1 &
```

## 📝 使用说明

### 访问Web界面

服务启动后,打开浏览器访问:
- **本地访问**: `http://localhost:5000`
- **远程访问**: `http://your-server-ip:5000`

### 上传文件

1. 点击 "📤 上传文件" 标签页
2. 点击上传区域选择文件,或直接拖拽文件到页面
3. 等待上传完成,页面会显示实时进度:
   - 上传进度百分比
   - 当前上传速度
   - 预计剩余时间
4. 上传完成后,文件保存在 `/var/uploads` 目录

### 下载文件

1. 点击 "📥 下载文件" 标签页
2. 浏览 `/var/downloads` 目录中的所有文件
3. 点击 "⬇️ 下载" 按钮开始下载
4. 支持断点续传功能:
   - 下载中断后可以继续
   - 无需重新开始下载

### 管理文件

- **查看文件信息**: 文件列表显示每个文件的大小、修改时间等详细信息
- **删除文件**: 点击 "🗑️ 删除" 按钮可以删除不需要的文件
- **刷新列表**: 点击 "🔄 刷新文件列表" 按钮更新文件列表

## 🔧 配置选项

### 环境变量配置

创建 `.env` 文件 (参考 `.env.example`) 来自定义配置:

```bash
# 复制示例配置
cp .env.example .env

# 编辑配置文件
nano .env
```

**可配置项:**

| 配置项 | 说明 | 默认值 |
|--------|------|--------|
| `UPLOAD_FOLDER` | 上传文件存储目录 | `/var/uploads` |
| `DOWNLOAD_FOLDER` | 可下载文件目录 | `/var/downloads` |
| `CHUNK_SIZE` | 分片大小 (字节) | `104857600` (100MB) |
| `PORT` | 服务监听端口 | `5000` |
| `HOST` | 服务监听地址 | `0.0.0.0` |

### 修改分片大小

根据网络状况调整分片大小可以优化传输性能:

**1. 编辑 `app.py`:**
```python
CHUNK_SIZE = 200 * 1024 * 1024  # 改为 200MB
```

**2. 编辑 `templates/index.html`:**
```javascript
const CHUNK_SIZE = 200 * 1024 * 1024; // 200MB
```

**分片大小建议:**
- **慢速网络**: 50MB (52428800 字节)
- **普通网络**: 100MB (104857600 字节) - 默认
- **快速网络**: 200MB (209715200 字节)
- **内网环境**: 500MB (524288000 字节)

### 修改服务端口

编辑 `app.py` 文件最后一行:

```python
app.run(host='0.0.0.0', port=8080, threaded=True, debug=False)
```

同时修改 `install_service.sh` 中的端口配置(如果使用systemd服务)。

### 配置反向代理 (Nginx)

```nginx
server {
    listen 80;
    server_name your-domain.com;

    client_max_body_size 100G;
    proxy_read_timeout 600s;
    proxy_connect_timeout 600s;
    proxy_send_timeout 600s;

    location / {
        proxy_pass http://127.0.0.1:5000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        # 支持断点续传
        proxy_pass_request_headers on;
        proxy_buffering off;
    }
}
```

### 配置 SSL/HTTPS

使用 Let's Encrypt 获取免费SSL证书:

```bash
sudo apt install certbot python3-certbot-nginx -y
sudo certbot --nginx -d your-domain.com
```

## 🔒 安全建议

1. **配置防火墙**
   ```bash
   sudo ufw allow 5000/tcp  # 如果不使用Nginx
   sudo ufw enable
   ```

2. **设置认证** (可选)
   在 `app.py` 中添加HTTP基本认证

3. **限制访问IP** (可选)
   使用Nginx配置 `allow`/`deny` 规则

4. **定期清理临时文件**
   ```bash
   # 添加到 crontab
   0 3 * * * rm -rf /var/uploads/temp/*
   ```

## 📊 性能优化

### 1. 使用 Gunicorn (生产环境推荐)

```bash
# 安装 Gunicorn
pip3 install gunicorn

# 启动服务 (4个工作进程)
gunicorn -w 4 -b 0.0.0.0:5000 app:app
```

### 2. 系统参数优化

编辑 `/etc/sysctl.conf`:

```bash
# 增加文件描述符限制
fs.file-max = 100000

# 优化TCP连接
net.core.somaxconn = 1024
net.ipv4.tcp_max_syn_backlog = 2048

# 保存并应用
sudo sysctl -p
```

### 3. 文件系统优化

```bash
# 使用 XFS 或 ext4 文件系统
# 挂载时添加优化选项
sudo mount -o noatime,nodiratime /dev/sdX /var/uploads
```

## 🐛 故障排除

### 问题1: 上传中断

- 检查网络连接稳定性
- 增加Nginx的 `proxy_read_timeout` 配置
- 查看服务器日志: `tail -f server.log`

### 问题2: 磁盘空间不足

- 清理临时文件: `rm -rf /var/uploads/temp/*`
- 检查磁盘空间: `df -h`
- 扩容磁盘或清理不需要的文件

### 问题3: 权限错误

- 确保目录权限正确: `ls -la /var/uploads`
- 检查文件所有者: `chown -R www-data:www-data /var/uploads`

## 📝 API 文档

### 初始化上传
```
POST /api/upload/init
Content-Type: application/json

{
  "filename": "example.zip",
  "filesize": 1234567890,
  "file_hash": "optional_md5_hash"
}
```

### 上传分片
```
POST /api/upload/chunk
Content-Type: multipart/form-data

upload_id: xxx
chunk_index: 0
total_chunks: 10
chunk: [binary data]
```

### 完成上传
```
POST /api/upload/complete
Content-Type: application/json

{
  "upload_id": "xxx"
}
```

### 下载文件
```
GET /api/download/<filepath>
Range: bytes=0-1048575  # 可选,用于断点续传
```

### 列出文件
```
GET /api/list
```

### 删除文件
```
DELETE /api/delete/<filepath>
```

## 📄 许可证

MIT License

## 🤝 贡献

欢迎提交 Issue 和 Pull Request!

## 📞 支持

如有问题,请提交 Issue 或联系管理员。

---

## ⚡ 快速参考

### 常用命令速查

```bash
# 启动服务
./start.sh

# 停止服务
./stop.sh

# 重启服务
./restart.sh

# 查看日志
tail -f server.log

# 安装为系统服务
sudo ./install_service.sh

# 系统服务管理
sudo systemctl status file-transfer    # 查看状态
sudo systemctl start file-transfer     # 启动服务
sudo systemctl stop file-transfer      # 停止服务
sudo systemctl restart file-transfer   # 重启服务
sudo journalctl -u file-transfer -f    # 查看日志
```

### 文件位置

| 类型 | 路径 | 说明 |
|------|------|------|
| 上传文件 | `/var/uploads/` | 用户上传的文件存储位置 |
| 下载文件 | `/var/downloads/` | 可供下载的文件目录 |
| 临时文件 | `/var/uploads/temp/` | 上传过程中的临时分片 |
| 服务日志 | `./server.log` | 应用运行日志 |
| 系统日志 | `journalctl -u file-transfer` | systemd服务日志 |

### 网络访问

- **默认端口**: 5000
- **本地访问**: http://localhost:5000
- **远程访问**: http://your-server-ip:5000
- **防火墙规则**: `sudo ufw allow 5000/tcp` (如果不使用Nginx)

---

**注意**: 本系统专为内网或可信环境设计,生产环境使用请务必配置防火墙和身份认证。

