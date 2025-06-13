#include <iostream>
#include <thread>
#include <future>
#include <functional>
#include <chrono>

// 一个简单的函数，用于演示
int multiply(int a, int b) {
    std::cout << "Packaged task start"<< std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Packaged task end"<< std::endl;
    return a * b;

}

int main() {
    // 创建一个 packaged_task，包装函数 multiply
    std::packaged_task<int(int, int)> task(multiply);

    // 获取与 task 相关联的 future
    std::future<int> result = task.get_future();

    // 将任务交给另一个线程去执行
    std::thread t(std::move(task), 5, 6);
    
    std::cout << "Doing some work in the main thread"<< std::endl; // 输出 "Result: 30"
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "Finish work in the main thread"<< std::endl; // 输出 "Result: 30"

    // 等待任务完成并获取结果
    int value = result.get();

    std::cout << "Result: " << value << std::endl; // 输出 "Result: 30"

    // 等待线程结束
    t.join();

    return 0;
}

// g++ -std=c++20 -Wall packaged_task_2.cpp -o packaged_task_2.tsk