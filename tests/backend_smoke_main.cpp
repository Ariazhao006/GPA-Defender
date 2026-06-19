#include <iostream>

bool runBackendLoopSmokeTest();

int main() {
    if (!runBackendLoopSmokeTest()) {
        std::cout << "[Test] Backend loop smoke test failed.\n";
        return 1;
    }

    std::cout << "[Test] Backend loop smoke test passed.\n";
    return 0;
}
