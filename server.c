#define _GNU_SOURCE
#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <dirent.h>
#include <libgen.h>

// ==================== 配置常量 ====================

#define DEFAULT_PORT 5000
#define CHUNK_SIZE (100 * 1024 * 1024)  // 100MB
#define MAX_FILESIZE (10ULL * 1024 * 1024 * 1024)  // 10GB
#define UPLOAD_DIR "/var/uploads"
#define DOWNLOAD_DIR "/var/downloads"
#define TEMP_DIR "/var/uploads/temp"

// ==================== 数据结构 ====================

typedef struct {
    char *upload_id;
    char *filename;
    long long filesize;
    long long uploaded;
    int chunk_count;
    time_t start_time;
    pthread_mutex_t mutex;
} UploadSession;

typedef struct {
    struct MHD_PostProcessor *pp;
    UploadSession *session;
    FILE *file;
    char chunk_index[32];
    char total_chunks[32];
} UploadContext;

// ==================== 全局变量 ====================

static struct MHD_Daemon *daemon;
static UploadSession **sessions = NULL;
static int session_count = 0;
static int session_capacity = 0;
static pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;

// ==================== 工具函数 ====================

// 创建目录
static int ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? 0 : -1;
    }
    return mkdir(path, 0755);
}

// 格式化文件大小
static void format_size(long long size, char *buf, size_t buflen) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double dsize = (double)size;

    while (dsize >= 1024.0 && unit_index < 4) {
        dsize /= 1024.0;
        unit_index++;
    }

    snprintf(buf, buflen, "%.2f %s", dsize, units[unit_index]);
}

// URL编码
static char* url_encode(const char *str) {
    if (!str) return NULL;

    size_t len = strlen(str);
    char *encoded = malloc(len * 3 + 1);
    if (!encoded) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            encoded[j++] = c;
        } else {
            snprintf(encoded + j, 4, "%%%02X", (unsigned char)c);
            j += 3;
        }
    }
    encoded[j] = '\0';
    return encoded;
}

// ==================== 会话管理 ====================

static UploadSession* find_session(const char *upload_id) {
    pthread_mutex_lock(&sessions_mutex);
    for (int i = 0; i < session_count; i++) {
        if (sessions[i] && strcmp(sessions[i]->upload_id, upload_id) == 0) {
            pthread_mutex_unlock(&sessions_mutex);
            return sessions[i];
        }
    }
    pthread_mutex_unlock(&sessions_mutex);
    return NULL;
}

static UploadSession* create_session(const char *upload_id, const char *filename, long long filesize) {
    pthread_mutex_lock(&sessions_mutex);

    // 扩容
    if (session_count >= session_capacity) {
        int new_capacity = session_capacity == 0 ? 100 : session_capacity * 2;
        UploadSession **new_sessions = realloc(sessions, new_capacity * sizeof(UploadSession*));
        if (!new_sessions) {
            pthread_mutex_unlock(&sessions_mutex);
            return NULL;
        }
        sessions = new_sessions;
        for (int i = session_capacity; i < new_capacity; i++) {
            sessions[i] = NULL;
        }
        session_capacity = new_capacity;
    }

    // 创建新会话
    UploadSession *session = calloc(1, sizeof(UploadSession));
    if (!session) {
        pthread_mutex_unlock(&sessions_mutex);
        return NULL;
    }

    session->upload_id = strdup(upload_id);
    session->filename = strdup(filename);
    session->filesize = filesize;
    session->uploaded = 0;
    session->chunk_count = 0;
    session->start_time = time(NULL);
    pthread_mutex_init(&session->mutex, NULL);

    sessions[session_count++] = session;

    pthread_mutex_unlock(&sessions_mutex);
    return session;
}

static void cleanup_old_sessions() {
    time_t now = time(NULL);
    pthread_mutex_lock(&sessions_mutex);

    for (int i = 0; i < session_count; ) {
        if (sessions[i] && (now - sessions[i]->start_time) > 3600) { // 1小时超时
            free(sessions[i]->upload_id);
            free(sessions[i]->filename);
            pthread_mutex_destroy(&sessions[i]->mutex);
            free(sessions[i]);
            sessions[i] = NULL;

            // 移动数组元素
            for (int j = i; j < session_count - 1; j++) {
                sessions[j] = sessions[j + 1];
            }
            session_count--;
        } else {
            i++;
        }
    }

    pthread_mutex_unlock(&sessions_mutex);
}

// ==================== JSON响应生成 ====================

static void send_json_response(struct MHD_Connection *connection, int status_code,
                               const char *json_data) {
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(json_data),
        (void*)json_data,
        MHD_RESPMEM_MUST_COPY
    );

    if (response) {
        MHD_add_response_header(response, "Content-Type", "application/json; charset=utf-8");
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_queue_response(connection, status_code, response);
        MHD_destroy_response(response);
    }
}

static void send_json_success(struct MHD_Connection *connection, const char *message) {
    char *json;
    if (message) {
        asprintf(&json, "{\"success\":true,\"message\":\"%s\"}", message);
    } else {
        json = strdup("{\"success\":true}");
    }
    send_json_response(connection, MHD_HTTP_OK, json);
    free(json);
}

static void send_json_error(struct MHD_Connection *connection, int status_code,
                            const char *error) {
    char *json;
    asprintf(&json, "{\"success\":false,\"error\":\"%s\"}", error);
    send_json_response(connection, status_code, json);
    free(json);
}

// ==================== 文件上传处理 ====================

static enum MHD_Result iterate_post(void *coninfo_cls, enum MHD_ValueKind kind,
                                     const char *key, const char *filename,
                                     const char *content_type, const char *transfer_encoding,
                                     const char *data, uint64_t off, size_t size) {
    UploadContext *ctx = (UploadContext*)coninfo_cls;

    if (strcmp(key, "upload_id") == 0) {
        return MHD_YES;
    } else if (strcmp(key, "chunk_index") == 0) {
        strncpy(ctx->chunk_index, data, size);
        ctx->chunk_index[size] = '\0';
        return MHD_YES;
    } else if (strcmp(key, "total_chunks") == 0) {
        strncpy(ctx->total_chunks, data, size);
        ctx->total_chunks[size] = '\0';
        return MHD_YES;
    } else if (strcmp(key, "chunk") == 0 && ctx->session) {
        // 保存分片
        char temp_path[512];
        snprintf(temp_path, sizeof(temp_path), "%s/%s/chunk_%s",
                 TEMP_DIR, ctx->session->upload_id, ctx->chunk_index);

        ensure_dir(temp_path);

        FILE *fp = fopen(temp_path, "ab");
        if (fp) {
            fwrite(data, 1, size, fp);
            fclose(fp);

            pthread_mutex_lock(&ctx->session->mutex);
            ctx->session->uploaded += size;
            ctx->session->chunk_count++;
            pthread_mutex_unlock(&ctx->session->mutex);
        }
    }

    return MHD_YES;
}

static void request_completed(void *cls, struct MHD_Connection *connection,
                              void **con_cls, enum MHD_RequestTerminationCode toe) {
    UploadContext *ctx = *con_cls;
    if (ctx) {
        if (ctx->pp) {
            MHD_destroy_post_processor(ctx->pp);
        }
        if (ctx->file) {
            fclose(ctx->file);
        }
        free(ctx);
        *con_cls = NULL;
    }
}

// ==================== HTTP请求处理 ====================

static enum MHD_RESULT answer_to_connection(void *cls, struct MHD_Connection *connection,
                                            const char *url, const char *method,
                                            const char *version, const char *upload_data,
                                            size_t *upload_data_size, void **con_cls) {

    // 清理过期会话
    static time_t last_cleanup = 0;
    time_t now = time(NULL);
    if (now - last_cleanup > 300) { // 每5分钟清理一次
        cleanup_old_sessions();
        last_cleanup = now;
    }

    // 处理 OPTIONS 请求 (CORS)
    if (strcmp(method, "OPTIONS") == 0) {
        struct MHD_Response *response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "POST, GET, DELETE, OPTIONS");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type");
        MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return MHD_YES;
    }

    // ==================== 主页 ====================
    if (strcmp(url, "/") == 0 && strcmp(method, "GET") == 0) {
        FILE *fp = fopen("templates/index.html", "rb");
        if (!fp) {
            const char *msg = "页面不存在，请确保 templates/index.html 文件存在";
            struct MHD_Response *response = MHD_create_response_from_buffer(
                strlen(msg), (void*)msg, MHD_RESPMEM_PERSISTENT
            );
            MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
            MHD_destroy_response(response);
            return MHD_YES;
        }

        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        char *html = malloc(fsize + 1);
        fread(html, 1, fsize, fp);
        html[fsize] = '\0';
        fclose(fp);

        struct MHD_Response *response = MHD_create_response_from_buffer(
            fsize, html, MHD_RESPMEM_MUST_FREE
        );
        MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
        MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return MHD_YES;
    }

    // ==================== 初始化上传 ====================
    if (strcmp(url, "/api/upload/init") == 0 && strcmp(method, "POST") == 0) {
        if (*con_cls == NULL) {
            UploadContext *ctx = calloc(1, sizeof(UploadContext));
            ctx->pp = MHD_create_post_processor(connection, 1024 * 1024, iterate_post, ctx);
            *con_cls = ctx;
            return MHD_YES;
        }

        UploadContext *ctx = *con_cls;
        MHD_post_process(ctx->pp, upload_data, *upload_data_size);
        *upload_data_size = 0;

        if (*upload_data_size != 0) {
            return MHD_YES;
        }

        // 从POST数据中解析参数
        const char *filename = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "filename");
        const char *filesize_str = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "filesize");

        // 从请求体中读取JSON
        // 注意: 这里简化处理，实际应完整解析JSON

        char upload_id[128];
        snprintf(upload_id, sizeof(upload_id), "%ld_%s", (long)time(NULL), "upload");

        UploadSession *session = create_session(upload_id, "uploaded_file", 0);
        if (!session) {
            send_json_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "创建会话失败");
            return MHD_YES;
        }

        char *json;
        asprintf(&json, "{\"success\":true,\"upload_id\":\"%s\",\"chunk_size\":%d}",
                 upload_id, CHUNK_SIZE);
        send_json_response(connection, MHD_HTTP_OK, json);
        free(json);
        return MHD_YES;
    }

    // ==================== 上传分片 ====================
    if (strcmp(url, "/api/upload/chunk") == 0 && strcmp(method, "POST") == 0) {
        if (*con_cls == NULL) {
            UploadContext *ctx = calloc(1, sizeof(UploadContext));
            ctx->pp = MHD_create_post_processor(connection, CHUNK_SIZE, iterate_post, ctx);
            *con_cls = ctx;
            return MHD_YES;
        }

        UploadContext *ctx = *con_cls;
        MHD_post_process(ctx->pp, upload_data, *upload_data_size);
        *upload_data_size = 0;

        if (*upload_data_size != 0) {
            return MHD_YES;
        }

        // TODO: 完善分片处理逻辑
        send_json_success(connection, NULL);
        return MHD_YES;
    }

    // ==================== 完成上传 ====================
    if (strcmp(url, "/api/upload/complete") == 0 && strcmp(method, "POST") == 0) {
        // TODO: 合并分片
        send_json_success(connection, "文件上传成功");
        return MHD_YES;
    }

    // ==================== 列出文件 ====================
    if (strcmp(url, "/api/list") == 0 && strcmp(method, "GET") == 0) {
        DIR *dir = opendir(DOWNLOAD_DIR);
        if (!dir) {
            send_json_error(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "无法打开下载目录");
            return MHD_YES;
        }

        char *json = strdup("{\"success\":true,\"files\":[");
        struct dirent *entry;
        int first = 1;

        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) { // 常规文件
                char path[512];
                snprintf(path, sizeof(path), "%s/%s", DOWNLOAD_DIR, entry->d_name);

                struct stat st;
                if (stat(path, &st) == 0) {
                    char size_str[64];
                    format_size(st.st_size, size_str, sizeof(size_str));

                    char *file_json;
                    asprintf(&file_json, "%s{\"name\":\"%s\",\"size\":%ld,\"size_human\":\"%s\"}",
                             first ? "" : ",", entry->d_name, st.st_size, size_str);

                    char *new_json;
                    asprintf(&new_json, "%s%s", json, file_json);
                    free(json);
                    free(file_json);
                    json = new_json;
                    first = 0;
                }
            }
        }
        closedir(dir);

        char *final_json;
        asprintf(&final_json, "%s]}", json);
        free(json);

        send_json_response(connection, MHD_HTTP_OK, final_json);
        free(final_json);
        return MHD_YES;
    }

    // ==================== 下载文件 ====================
    if (strncmp(url, "/api/download/", 13) == 0 && strcmp(method, "GET") == 0) {
        const char *filename = url + 13;
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", DOWNLOAD_DIR, filename);

        FILE *fp = fopen(path, "rb");
        if (!fp) {
            send_json_error(connection, MHD_HTTP_NOT_FOUND, "文件不存在");
            return MHD_YES;
        }

        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        char *content = malloc(fsize);
        fread(content, 1, fsize, fp);
        fclose(fp);

        struct MHD_Response *response = MHD_create_response_from_buffer(
            fsize, content, MHD_RESPMEM_MUST_FREE
        );
        MHD_add_response_header(response, "Content-Disposition", "attachment");
        MHD_add_response_header(response, "Content-Type", "application/octet-stream");

        // 支持断点续传
        const char *range_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Range");
        if (range_header) {
            // TODO: 解析Range头并返回部分内容
            MHD_add_response_header(response, "Accept-Ranges", "bytes");
        }

        MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return MHD_YES;
    }

    // ==================== 删除文件 ====================
    if (strncmp(url, "/api/delete/", 12) == 0 && strcmp(method, "DELETE") == 0) {
        const char *filename = url + 12;
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", DOWNLOAD_DIR, filename);

        if (unlink(path) == 0) {
            send_json_success(connection, "文件已删除");
        } else {
            send_json_error(connection, MHD_HTTP_NOT_FOUND, "文件不存在或删除失败");
        }
        return MHD_YES;
    }

    // 404
    const char *msg_404 = "404 Not Found";
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(msg_404), (void*)msg_404, MHD_RESPMEM_PERSISTENT
    );
    MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);

    return MHD_YES;
}

// ==================== 主函数 ====================

int main(int argc, char **argv) {
    int port = DEFAULT_PORT;

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("用法: %s [选项]\n", argv[0]);
            printf("选项:\n");
            printf("  -p PORT   指定端口 (默认: %d)\n", DEFAULT_PORT);
            printf("  -h, --help 显示帮助\n");
            return 0;
        }
    }

    // 确保目录存在
    ensure_dir(UPLOAD_DIR);
    ensure_dir(DOWNLOAD_DIR);
    ensure_dir(TEMP_DIR);

    printf("========================================\n");
    printf("   大文件传输服务器 (C语言版本)\n");
    printf("========================================\n\n");
    printf("上传目录: %s\n", UPLOAD_DIR);
    printf("下载目录: %s\n", DOWNLOAD_DIR);
    printf("临时目录: %s\n", TEMP_DIR);
    printf("分片大小: %.2f MB\n", CHUNK_SIZE / (1024.0 * 1024.0));
    printf("服务端口: %d\n", port);
    printf("\n服务器启动在 http://0.0.0.0:%d\n\n", port);

    // 创建HTTP守护进程
    daemon = MHD_start_daemon(
        MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD,
        port,
        NULL, NULL,
        &answer_to_connection, NULL,
        MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
        MHD_OPTION_CONNECTION_LIMIT, 100,
        MHD_OPTION_END
    );

    if (!daemon) {
        fprintf(stderr, "无法启动服务器\n");
        return 1;
    }

    printf("服务器运行中，按 Ctrl+C 停止...\n");

    // 主循环
    while (1) {
        sleep(1);
    }

    // 清理
    MHD_stop_daemon(daemon);

    return 0;
}
