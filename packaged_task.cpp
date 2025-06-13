#include <iostream>
#include <future>
#include <deque>


int factorial(int N) {
    int res = 1;
    for (int i =N; i>0 ; --i) {
        res*=i;
    }
    std::cout<< "param:" << N << " Child thread result is:" <<res << std::endl;
    return res;
}

std::deque<std::packaged_task<int()>> tasks_queue;
std::mutex mu;
std::condition_variable cond_var;


void thread_1() {
    // worker to comsume the task
    std::cout << "thread_1" << std::endl;
    std::packaged_task<int()> t;
    {
        std::unique_lock<std::mutex> locker(mu);
        cond_var.wait(locker, []() {return !tasks_queue.empty();});
        t = std::move(tasks_queue.front());
        tasks_queue.pop_front();
    }
    t();
}
int main() {
    // the packaged task can linked a callable object with a future.
    std::thread t(factorial, 6);
    std::packaged_task<int(int)> pt(factorial);  // can not pass additional parameters
    // I can want to pass additional parameter, I have to do this. 
    std::packaged_task<int(int)> pt2([](int x) {return factorial(x);});
    std::packaged_task<int()> pt3(std::bind(factorial, 10)); // bind the first parameter
    
    pt(4);  //in a different context, other than the one that created this task
    pt2(6);
    pt3();
    std::cout<< "In main thread result is:" << pt2.get_future().get() << std::endl;
    std::cout<< "In main thread result is:" << pt3.get_future().get() << std::endl;


    // we can also use the packaged task work with a queue.
    std::thread t1(thread_1);
    std::packaged_task<int()> pt4([]() {return factorial(5);});
    std::future<int> fu = pt4.get_future();
    {
        std::lock_guard<std::mutex> locker(mu);
        tasks_queue.push_back(std::move(pt4));
    }
    cond_var.notify_one();
    std::cout<< " Future " << fu.get() << std::endl;
    t1.join();
    return 0;
}

// 3 wyas to get a future:
// promise::get_future()
// packaged_task::get_future()
// async() return a future

// g++ -std=c++20 -Wall packaged_task.cpp -o packaged_task.tsk