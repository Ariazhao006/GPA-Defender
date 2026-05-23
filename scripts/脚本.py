import os

def merge_code_files(root_dir, output_file, allowed_extensions):
    # 需要跳过的文件夹
    ignore_dirs = {'.git', '.vscode', '__pycache__', 'build', 'bin', 'obj', '.idea'}

    # 最终输出的汇总文件统一使用 utf-8 编码，方便我阅读
    with open(output_file, 'w', encoding='utf-8') as outfile:
        for dirpath, dirnames, filenames in os.walk(root_dir):
            dirnames[:] = [d for d in dirnames if d not in ignore_dirs]

            for filename in filenames:
                ext = os.path.splitext(filename)[1].lower()
                if ext in allowed_extensions:
                    filepath = os.path.join(dirpath, filename)
                    rel_path = os.path.relpath(filepath, root_dir)

                    content = ""
                    # 第一步：先尝试用 utf-8 读取
                    try:
                        with open(filepath, 'r', encoding='utf-8') as infile:
                            content = infile.read()
                    except UnicodeDecodeError:
                        # 第二步：如果报编码错误，回退到国内 Windows 常见的 gb18030 (兼容 gbk) 读取
                        try:
                            with open(filepath, 'r', encoding='gb18030') as infile:
                                content = infile.read()
                        except Exception as e:
                            print(f"无法读取文件 {rel_path}，已跳过。错误信息: {e}")
                            continue
                    except Exception as e:
                        print(f"无法读取文件 {rel_path}，已跳过。错误信息: {e}")
                        continue

                    # 成功读取后写入合并文件
                    outfile.write(f"// " + "="*40 + "\n")
                    outfile.write(f"// File: {rel_path}\n")
                    outfile.write(f"// " + "="*40 + "\n")
                    
                    outfile.write("```cpp\n")
                    outfile.write(content)
                    
                    if not content.endswith('\n'):
                        outfile.write("\n")
                    outfile.write("```\n\n")

if __name__ == "__main__":
    PROJECT_ROOT = "."
    OUTPUT_FILENAME = "merged_code.txt"

    # 只提取 C++ 相关文件
    EXTENSIONS = {'.cpp', '.h', '.hpp', '.c'}

    print("开始合并代码文件...")
    merge_code_files(PROJECT_ROOT, OUTPUT_FILENAME, EXTENSIONS)
    print(f"合并完成！请打开同目录下的 {OUTPUT_FILENAME} 复制内容。")