# Makefile for Large File Transfer Server (C Version)

# 编译器和选项
CC = gcc
CFLAGS = -Wall -O2 -std=gnu99 -D_GNU_SOURCE
LDFLAGS = -lmicrohttpd -lpthread

# 目标文件
TARGET = fileserver

# 源文件
SOURCES = server.c
OBJECTS = $(SOURCES:.c=.o)

# 安装目录
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

# 默认目标
all: $(TARGET)

# 编译
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "编译完成: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 安装
install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)
	@echo "安装完成: $(BINDIR)/$(TARGET)"

# 卸载
uninstall:
	rm -f $(BINDIR)/$(TARGET)
	@echo "卸载完成"

# 清理
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "清理完成"

# 调试版本
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# 运行
run: $(TARGET)
	./$(TARGET)

# 帮助
help:
	@echo "可用的make目标:"
	@echo "  all        - 编译程序 (默认)"
	@echo "  install    - 安装程序到系统"
	@echo "  uninstall  - 从系统卸载程序"
	@echo "  clean      - 清理编译文件"
	@echo "  debug      - 编译调试版本"
	@echo "  run        - 编译并运行"
	@echo "  help       - 显示此帮助信息"

.PHONY: all install uninstall clean debug run help
