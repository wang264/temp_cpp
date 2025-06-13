
#include <vector>
#include <thread>
#include <mutex>


//one producer， multiple consumer
template<typename T>
class BlockingQueue {
public:
    BlockingQueue(bool nonblock=false): nonblock_(nonblock) {}
    // enqueu
    void Push(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(value);
        not_empty_.notify_one();
    }

    // pop
    bool Pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this] {return (!queue_.empty() || nonblock_);});
    
        if (queue_.empty()){
            return false;
        }
        value = queue_.front();
        queue_.pop();
        return true;
    }

    // 解除阻塞在当前队列的线程
    void Cancel() {
        std::lock_guard<std::mutex> lock(mutex_);
        nonblock_ = true;  // make it unblock
        not_empty_.notify_all(); //notify the thread that is currently blocking
    }

private:
    bool nonblock_;
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable not_empty_;
};


//multiple producer，multiple consumer. consumers and produces each own a queue
template <typename T>
class BlockingQueuePro {
public:
    BlockingQueuePro(bool nonblock = false): nonblock_(nonblock) {}

    void Push(const T& value) {
        std::lock_guard<std::mutex> lock(producer_mutex_);
        producer_queue_.push(value);
        producer_q_not_empty_.notify_one();
    }

    bool Pop(T& value) {
        std::unique_lock<std::mutex> lock(consumer_mutex_);
        if (consumer_queue_.empty() && SwapQueue_() == 0) { //if consumer queue is empty, start to swap.
            return false; // thread will exit. 
        }
        value = consumer_queue_.front();
        consumer_queue_.pop();
        return true;
    }

    void Cancel() { //解除阻塞
        std::lock_guard<std::mutex> lock(producer_mutex_);
        nonblock_ = true;
        producer_q_not_empty_.notify_all();
    }
private:
    int SwapQueue_() {
        std::unique_lock<std::mutex> lock(producer_mutex_);
        // we swap queue when consumer queue is empty, if producer queue is also empty, then we will block it. 
        producer_q_not_empty_.wait(lock, [this] {return !producer_queue_.empty() || nonblock_; }); 
        std::swap(producer_queue_, consumer_queue_);
        return consumer_queue_.size();
    }
    bool nonblock_;
    std::queue<T> producer_queue_;
    std::queue<T> consumer_queue_;
    std::mutex producer_mutex_;
    std::mutex consumer_mutex_;
    std::condition_variable producer_q_not_empty_;

};

class ThreadPool {
public:
    // constructor
    explicit ThreadPool(int num_threads) {
        for (size_t i =0; i<num_threads; ++i) {
            workers_.emplace_back([this] {Worker();});
        }
    }
    // destructor
    ~ThreadPool() {
        task_queue_.Cancel();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    // Post task to thread pool
    // F: callable object
    template<typename F, typename... Args>
    void Post(F&& f, Args &&...args) {
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        task_queue_.Push(task);
    }
private:
    void Worker() {
        while (true) {
            std::function<void()> task;
            if (!task_queue_.Pop(task)) {
                break;
            }
            task();
        }
    }

    BlockingQueuePro<std::function<void()>> task_queue_;
    std::vector<std::thread> workers_;
};


// g++ -std=c++20 -o thread_pool.tsk thread_pool.cpp 