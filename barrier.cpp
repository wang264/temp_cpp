#include <iostream>
#include <thread>
#include <barrier>
#include <vector>
#include <mutex>
#include <sstream>

//std::barrier is a synchronization primitive in C++20 that allows multiple threads to wait at a shared point
//in their execution. It's similar to std::latch, but std::barrier is reusable and supports an optional completion function. 

//https://cppreference.com/w/cpp/thread/barrier.html

std::mutex mtx;

void print(const std::string& str) {
    mtx.lock();
    std::cout << str << std::endl;
    mtx.unlock();
}
void worker(int id, std::barrier<>& sync_point) {
    std::stringstream ss;
    ss<< "Worker " << id << " is doing phase 1 work";
    print(ss.str());
    std::this_thread::sleep_for(std::chrono::milliseconds(100 * id)); // simulate actual work time

    // arrive to the barrier, wait for other thread
    sync_point.arrive_and_wait();
    std::stringstream ss2;
    ss2 << "Worker " << id << " has completed phase 1 and is doing phase 2 work";
    print(ss2.str());
    std::this_thread::sleep_for(std::chrono::milliseconds(100 * id)); // simulate actual work time

    sync_point.arrive_and_wait();
    
    std::stringstream ss3;
    ss3 << "Worker " << id << " has completed phase 2 and is doing phase 3 work";
    print(ss3.str());
    std::this_thread::sleep_for(std::chrono::milliseconds(100 * id)); // simulate actual work time
    
    std::stringstream ss4;
    ss4 << "Worker " << id << " has completed phase 3 work";
    print(ss4.str());
}

void worker_only2phase(int id, std::barrier<>& sync_point) {
    std::stringstream ss;
    ss<< "Worker " << id << " is doing phase 1 work";
    print(ss.str());
    std::this_thread::sleep_for(std::chrono::milliseconds(100 * id)); // simulate actual work time

    // arrive to the barrier, wait for other thread
    sync_point.arrive_and_wait();
    std::stringstream ss2;
    ss2 << "Worker " << id << " has completed phase 1 and is doing phase 2 work";
    print(ss2.str());
    std::this_thread::sleep_for(std::chrono::milliseconds(100 * id)); // simulate actual work time

    sync_point.arrive_and_drop(); // decreate count, and shrink the barrier paticipant count by 1
    std::stringstream ss3;
    ss3 << "Worker " << id << " only have 2 phase of work. Done";
    print(ss3.str());
}


int main() {
    const int num_threads = 5;
    std::barrier sync_point(6); // create a barrier，wait for all the thread to arrive

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i + 1, std::ref(sync_point));
    }
    // special 2 phase worker
    threads.emplace_back(worker_only2phase,6,std::ref(sync_point));

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "All workers have completed their tasks." << std::endl;

    return 0;
}

// g++ -std=c++20 -o barrier.tsk barrier.cpp