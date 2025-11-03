#include <iostream>
#include <chrono>
#include <functional>

template<typename  T>
void CaculateTime(std::function<void()> nameOfFunction) {
    auto startTime = std::chrono::steady_clock::now();
    nameOfFunction();
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<T>(endTime - startTime);
    std::cout << "timecost: " << duration.count() << std::endl;
    return;
}
