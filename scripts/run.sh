VM="./build/fast-debug/bin/meow-vm"
TEST_DIR="tests"

if [[ ! -x "$VM" ]]; then
    echo "❌ Không tìm thấy VM ở $VM"
    exit 1
fi

"$VM"
