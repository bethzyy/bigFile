#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
大文件上传下载服务器
支持 >10GB 文件的上传和下载,采用分片传输和断点续传
"""

import os
import json
import shutil
from flask import Flask, request, send_file, jsonify, render_template_string
from werkzeug.utils import secure_filename
import threading
import time

app = Flask(__name__)
app.config['MAX_CONTENT_LENGTH'] = None  # 取消上传大小限制

# 配置项
UPLOAD_FOLDER = os.getenv('UPLOAD_FOLDER', '/var/uploads')
DOWNLOAD_FOLDER = os.getenv('DOWNLOAD_FOLDER', '/var/downloads')
CHUNK_SIZE = 100 * 1024 * 1024  # 100MB 每个分片
ALLOWED_EXTENSIONS = {'txt', 'pdf', 'png', 'jpg', 'jpeg', 'gif', 'zip', 'rar', '7z',
                      'tar', 'gz', 'mp4', 'avi', 'mkv', 'mov', 'iso', 'img', 'bin',
                      'csv', 'xlsx', 'doc', 'docx', 'ppt', 'pptx'}

# 确保目录存在
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(DOWNLOAD_FOLDER, exist_ok=True)
os.makedirs(os.path.join(UPLOAD_FOLDER, 'temp'), exist_ok=True)

# 上传状态存储
upload_status = {}
lock = threading.Lock()


def allowed_file(filename):
    """检查文件扩展名"""
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS


def get_file_info(filepath):
    """获取文件信息"""
    if not os.path.exists(filepath):
        return None
    stat = os.stat(filepath)
    return {
        'name': os.path.basename(filepath),
        'size': stat.st_size,
        'size_human': sizeof_fmt(stat.st_size),
        'modified': time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(stat.st_mtime))
    }


def sizeof_fmt(num, suffix='B'):
    """格式化文件大小"""
    for unit in ['', 'Ki', 'Mi', 'Gi', 'Ti', 'Pi', 'Ei', 'Zi']:
        if abs(num) < 1024.0:
            return "%3.1f %s%s" % (num, unit, suffix)
        num /= 1024.0
    return "%.1f %s%s" % (num, 'Yi', suffix)


@app.route('/')
def index():
    """主页"""
    return render_template_string(open('templates/index.html', encoding='utf-8').read())


@app.route('/api/list', methods=['GET'])
def list_files():
    """列出可下载的文件"""
    try:
        files = []
        for root, dirs, filenames in os.walk(DOWNLOAD_FOLDER):
            for filename in filenames:
                filepath = os.path.join(root, filename)
                relpath = os.path.relpath(filepath, DOWNLOAD_FOLDER)
                info = get_file_info(filepath)
                if info:
                    info['path'] = relpath
                    files.append(info)
        return jsonify({'success': True, 'files': files})
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500


@app.route('/api/upload/init', methods=['POST'])
def init_upload():
    """初始化上传,获取上传ID"""
    try:
        data = request.json
        filename = secure_filename(data['filename'])
        filesize = int(data['filesize'])
        file_hash = data.get('file_hash', '')

        upload_id = f"{int(time.time())}_{filename}"

        with lock:
            upload_status[upload_id] = {
                'filename': filename,
                'filesize': filesize,
                'uploaded': 0,
                'chunks': {},
                'status': 'initializing'
            }

        return jsonify({
            'success': True,
            'upload_id': upload_id,
            'chunk_size': CHUNK_SIZE
        })
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500


@app.route('/api/upload/chunk', methods=['POST'])
def upload_chunk():
    """上传文件分片"""
    try:
        upload_id = request.form['upload_id']
        chunk_index = int(request.form['chunk_index'])
        total_chunks = int(request.form['total_chunks'])
        chunk = request.files['chunk']

        if upload_id not in upload_status:
            return jsonify({'success': False, 'error': 'Invalid upload ID'}), 400

        # 保存临时分片
        temp_dir = os.path.join(UPLOAD_FOLDER, 'temp', upload_id)
        os.makedirs(temp_dir, exist_ok=True)

        chunk_path = os.path.join(temp_dir, f'chunk_{chunk_index}')
        chunk.save(chunk_path)

        # 更新上传状态
        with lock:
            upload_status[upload_id]['chunks'][chunk_index] = chunk_path
            upload_status[upload_id]['uploaded'] += os.path.getsize(chunk_path)
            upload_status[upload_id]['status'] = 'uploading'

        return jsonify({
            'success': True,
            'uploaded': upload_status[upload_id]['uploaded'],
            'total': upload_status[upload_id]['filesize']
        })
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500


@app.route('/api/upload/complete', methods=['POST'])
def complete_upload():
    """完成上传,合并分片"""
    try:
        data = request.json
        upload_id = data['upload_id']

        if upload_id not in upload_status:
            return jsonify({'success': False, 'error': 'Invalid upload ID'}), 400

        status = upload_status[upload_id]
        temp_dir = os.path.join(UPLOAD_FOLDER, 'temp', upload_id)
        filename = status['filename']
        final_path = os.path.join(UPLOAD_FOLDER, filename)

        # 合并分片
        with open(final_path, 'wb') as outfile:
            chunk_indices = sorted([int(k.split('_')[1]) for k in os.listdir(temp_dir)])
            for idx in chunk_indices:
                chunk_path = os.path.join(temp_dir, f'chunk_{idx}')
                with open(chunk_path, 'rb') as infile:
                    shutil.copyfileobj(infile, outfile, CHUNK_SIZE)

        # 清理临时文件
        shutil.rmtree(temp_dir)

        # 更新状态
        with lock:
            status['status'] = 'completed'
            status['final_path'] = final_path

        return jsonify({'success': True, 'message': '文件上传成功'})
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500


@app.route('/api/upload/status/<upload_id>', methods=['GET'])
def get_upload_status(upload_id):
    """获取上传状态"""
    if upload_id not in upload_status:
        return jsonify({'success': False, 'error': 'Upload not found'}), 404

    status = upload_status[upload_id]
    return jsonify({
        'success': True,
        'status': status
    })


@app.route('/api/download/<path:filename>', methods=['GET'])
def download_file(filename):
    """下载文件,支持断点续传"""
    try:
        filepath = os.path.join(DOWNLOAD_FOLDER, filename)
        if not os.path.exists(filepath):
            return jsonify({'success': False, 'error': '文件不存在'}), 404

        # 支持断点续传
        range_header = request.headers.get('Range')
        if range_header:
            byte_range = range_header.replace('bytes=', '').split('-')
            start = int(byte_range[0]) if byte_range[0] else 0
            end = int(byte_range[1]) if len(byte_range) > 1 and byte_range[1] else None

            file_size = os.path.getsize(filepath)
            end = end if end else file_size - 1
            length = end - start + 1

            def generate():
                with open(filepath, 'rb') as f:
                    f.seek(start)
                    remaining = length
                    while remaining > 0:
                        chunk_size = min(CHUNK_SIZE, remaining)
                        data = f.read(chunk_size)
                        if not data:
                            break
                        remaining -= len(data)
                        yield data

            response = app.response_class(
                generate(),
                mimetype='application/octet-stream',
                direct_passthrough=True
            )
            response.headers.add('Content-Range', f'bytes {start}-{end}/{file_size}')
            response.headers.add('Accept-Ranges', 'bytes')
            response.headers.add('Content-Length', str(length))
            response.status_code = 206
            return response
        else:
            return send_file(filepath, as_attachment=True)
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500


@app.route('/api/delete/<path:filename>', methods=['DELETE'])
def delete_file(filename):
    """删除文件"""
    try:
        filepath = os.path.join(DOWNLOAD_FOLDER, filename)
        if os.path.exists(filepath):
            os.remove(filepath)
            return jsonify({'success': True, 'message': '文件已删除'})
        else:
            return jsonify({'success': False, 'error': '文件不存在'}), 404
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500


if __name__ == '__main__':
    print(f"上传目录: {UPLOAD_FOLDER}")
    print(f"下载目录: {DOWNLOAD_FOLDER}")
    print(f"分片大小: {sizeof_fmt(CHUNK_SIZE)}")
    print("\n服务器启动在 http://0.0.0.0:5000")
    app.run(host='0.0.0.0', port=5000, threaded=True, debug=False)
