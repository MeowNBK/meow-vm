// --- Old codes ---

#include <iostream>
#include "vm/meow_vm.h"

int main(int argc, char* argv[]) {
    try {
        // Giả sử entry point là thư mục hiện tại và file là "main.meow"
        // Trong tương lai, bạn sẽ phân tích argc/argv để lấy các đường dẫn này
        meow::vm::MeowVM vm(".", "main.meow", argc, argv);
        vm.interpret();
    } catch (const meow::vm::VMError& e) {
        std::cerr << "VM Runtime Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}