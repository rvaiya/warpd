import os

PROJECT_DIR = "D:\Code\warpd"

OUTPUT_FILE = "project_dump.txt"

# 想要包含的代码文件后缀
INCLUDE_EXT = {
    ".py", ".js", ".ts", ".tsx", ".jsx",
     ".yaml", ".yml", ".toml",
    ".md",  ".html", ".css",
    ".c", ".cpp", ".h", ".hpp",
    ".sh", ".bat", ".ps1", ".lua",".mk",
    #".txt",
    #".json",
}

# 忽略的目录
EXCLUDE_DIRS = {
    "__pycache__", "node_modules", ".git", ".idea", ".vscode",
    "dist", "build", "venv", ".venv", "env", ".env",
    "GroundingDINO", "Food-101-LT","results", "docs","crello_boris", "outputs",
}

def should_include_file(filename):
    ext = os.path.splitext(filename)[1].lower()
    return ext in INCLUDE_EXT

def should_skip_dir(dirname):
    return dirname.lower() in EXCLUDE_DIRS

def format_size(n_bytes):
    """把字节数格式化为更友好的字符串"""
    if n_bytes < 1024:
        return f"{n_bytes} B"
    elif n_bytes < 1024**2:
        return f"{n_bytes/1024:.2f} KB"
    else:
        return f"{n_bytes/1024**2:.2f} MB"

def export_project_to_txt(project_dir, output_file):
    with open(output_file, "w", encoding="utf-8") as out:
        for root, dirs, files in os.walk(project_dir):

            # 跳过无需遍历的目录
            dirs[:] = [d for d in dirs if not should_skip_dir(d)]

            # --- 计算当前目录的总大小 ---
            dir_total_size = 0
            for file in files:
                file_path = os.path.join(root, file)
                if os.path.isfile(file_path):
                    try:
                        dir_total_size += os.path.getsize(file_path)
                    except:
                        pass

            print(f"[DIR] {root} — size: {format_size(dir_total_size)}")

            for file in files:
                if not should_include_file(file):
                    continue

                file_path = os.path.join(root, file)

                # 获取单个文件大小
                try:
                    file_size = os.path.getsize(file_path)
                except:
                    file_size = -1

                print(f"  [FILE] {file_path} — {format_size(file_size)}")

                # 写入标题栏
                out.write("\n\n" + "="*80 + "\n")
                out.write(f"FILE: {file_path}\n")
                out.write(f"SIZE: {format_size(file_size)}\n")
                out.write("="*80 + "\n")

                # 写入文件内容
                try:
                    with open(file_path, "r", encoding="utf-8") as f:
                        out.write(f.read())
                except:
                    out.write("[无法读取该文件 - 可能编码问题]\n")

    print(f"\n导出完成！已写入: {output_file}\n")

if __name__ == "__main__":
    export_project_to_txt(PROJECT_DIR, OUTPUT_FILE)
