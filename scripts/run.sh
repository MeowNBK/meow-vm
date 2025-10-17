VM="./build/fast-debug/bin/meow-vm"
TEST_DIR="tests"

if [[ ! -x "$VM" ]]; then
    echo "❌ Không tìm thấy VM ở $VM"
    exit 1
fi

for f in "$TEST_DIR"/*; do
    filename=$(basename "$f")
    
    if [[ "$filename" != *.meowb ]]; then
        f="$f.meowb"
    fi

    if [[ -f "$f" ]]; then
        "$VM" "$f"
    fi
done
