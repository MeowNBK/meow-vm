#!/bin/bash

OUTPUT="meowscript.txt"

# Xóa file output cũ nếu có
rm -f "$OUTPUT"

# Hàm gom file .cpp và .h đệ quy
bundle_dir() {
    local dir="$1"
    for file in "$dir"/*; do
        if [[ -d "$file" ]]; then
            # Bỏ qua một số thư mục không cần thiết
            case "$file" in
                *.git*|*/build*|*/tests*|*/output*|*/cache*) continue ;;
            esac
            bundle_dir "$file"
        elif [[ -f "$file" && ( "$file" == *.cpp || "$file" == *.h ) ]]; then
            echo "===== FILE: $file =====" >> "$OUTPUT"
            cat "$file" >> "$OUTPUT"
            echo -e "\n\n" >> "$OUTPUT"
        fi
    done
}

echo "😼 Bắt đầu gom các file .cpp và .h..."
bundle_dir "."
echo "✨ Hoàn tất! File output: $OUTPUT"
