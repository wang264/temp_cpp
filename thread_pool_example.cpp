#include<iostream>
#include<thread>
#include<vector>
#include<atomic>
#include<chrono>
#include <random>
#include "thread_pool.h"

std::atomic<int> task_counter{0};

void Task(int id) {
    static thread_local std::mt19937 rng{ std::random_device{}() }; //one RNG per thread
    std::uniform_int_distribution<int> dist(0, 500);

    int extra_ms = dist(rng);
    std::this_thread::sleep_for(std::chrono::milliseconds(500+extra_ms));
    std::cout <<"Task:"<<id<<" executed by thread" << std::this_thread::get_id() << std::endl;
    task_counter++;
}

// producer thread function 
void Producer(ThreadPool& pool, int producer_id, int num_tasks) {
    for (int i=0; i<num_tasks; ++i) {
        int task_id = producer_id*1000 + i; // generate unique task ID.
        pool.Post(Task, task_id); // Post the task into the thread pool
        std::cout << "Producer " << producer_id << " posted task " << task_id << std::endl;
    }
}


int main() {
    constexpr int num_producers = 4; 
    constexpr int num_tasks_per_producer = 10;
    constexpr int num_threads_in_pool = 2; 

    ThreadPool pool(num_threads_in_pool);

    std::vector<std::thread> producers;

    // start producer thread
    for (int i=0; i<num_producers; ++i) {
        producers.emplace_back(Producer, std::ref(pool), i, num_tasks_per_producer);
    }

    // wait for all producer thread to complete
    for (auto& producer: producers) {
        producer.join();
    }

    // wait for all task complete.
    while (task_counter < num_producers*num_tasks_per_producer) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    std::cout << "All tasks executed: "<< task_counter << std::endl;
    return 0;
}

// g++ -std=c++20 -o thread_pool_example.tsk -l pthread thread_pool_example.cpp 