
#include <iostream>
#include <thread>

template <typename T>
class shared_ptr {
private:
    T* ptr;
    std::atomic<std::size_t>* ref_count;

    void release() {
        if (ref_count && ref_count->fetch_sub(1,std::memory_order_acq_rel) == 1){
            delete ptr;
            delete ref_count;
        }
    }
public:
    shared_ptr() : ptr(nullptr), ref_count(nullptr) {}
    //constructor   not allow shared_ptr<int> p = new int(10)   -> have to use shared_ptr<int> p = shared_ptr<int>(new int(10))
    explicit shared_ptr(T* p): ptr(p), ref_count(p ? new std::atomic<std::size_t>(1) : nullptr) {}
    //destructor
    ~shared_ptr(){release();}
    //copy constructor
    shared_ptr(const shared_ptr& other): ptr(other.ptr), ref_count(other.ref_count){
        if (ref_count) {
            ref_count->fetch_add(1,std::memory_order_relaxed);
        }
    }
    //assginement operator
    shared_ptr& operator=(const shared_ptr& other){
        if (this!= &other) {
            release();
            ptr = other.ptr;
            ref_count = other.ref_count;
            if (ref_count) {
                ref_count->fetch_add(1, std::memory_order_relaxed);
            }
        }
        return *this;
    }
    // move constructor
    shared_ptr(shared_ptr&& other) noexcept: ptr(other.ptr), ref_count(other.ref_count){
        other.ptr = nullptr;
        other.ref_count = nullptr;
    }
    // move assignment operator
    shared_ptr& operator=(shared_ptr&& other)  noexcept{
        if (this != &other) {
            release();
            ptr = other.ptr;
            ref_count = other.ref_count;
            other.ptr = nullptr;
            other.ref_count = nullptr;
        }
        return *this;
    }
    // use count
    std::size_t use_count() const {
        return ref_count ? ref_count->load(std::memory_order_acquire) : 0;
    }
    // dereference
    T& operator* () const {return *ptr;}
    // arrow operator
    T* operator-> () const {return ptr;}
    // bool
    operator bool() const { return ptr != nullptr; }
    // get
    T* get() const { return ptr; }
    // reset
    void reset(T* p = nullptr) {
        release();
        ptr = p;
        ref_count = p ? new std::atomic<std::size_t>(1) : nullptr;
    }
};

void test_shared_ptr_thread_safety() {
    shared_ptr<int> ptr(new int(42));

    const int num_threads = 10;
    std::vector<std::thread> threads;
    for (int i=0; i<num_threads; ++i) {
        threads.emplace_back([&ptr]() {
            for (int j=0; j<10000;++j){
                shared_ptr<int> local_ptr(ptr);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    for (auto& thread: threads) {
        thread.join();
    }

    std::cout<< "use_count:" <<ptr.use_count() << std::endl;
    if (ptr.use_count()==1) {
        std::cout << "Test Passed: shared_ptr is thread-safe" << std::endl;
    } else {
        std::cout << "Test failed: shared_ptr is not thread-safe!" << std::endl;
    }
}


int main() {
    shared_ptr<int> aa(new int(10));
    shared_ptr<int> bb = aa;

    std::cout << "shared_count of bb:" <<bb.use_count() << std::endl;
    std::cout << "shared_count of aa: " <<aa.use_count() << std::endl;
    aa.reset( new int(20));
    std::cout << "shared_count of bb:" <<bb.use_count() << std::endl;
    std::cout << "shared_count of aa: " <<aa.use_count() << std::endl;
    std::cout << " a:"<<*aa << "  b:" << *bb <<std::endl;

    shared_ptr<int> ptr1(new int(10));
    shared_ptr<int> ptr2(new int(20));
    shared_ptr<int> ptr3 = std::move(ptr2);
    std::cout << "shared_count of ptr3: " <<ptr3.use_count() << std::endl;
    // std::cout << "ptr2: " <<*ptr2 << std::endl; // seg fault, p2 already moved.
    std::cout << "ptr3: " <<*ptr3 << std::endl; 
    std::cout << "ptr3: " <<*(ptr3.get()) << std::endl; 
    ptr3.reset(new int(40));
    std::cout << "ptr3: " <<*ptr3 << std::endl; 
    shared_ptr<int> ptr4;
    std::cout << "ptr3 as bool:"<<static_cast<bool>(ptr3)<<std::endl;
    std::cout << "ptr4 as bool:"<<static_cast<bool>(ptr4)<<std::endl;


    test_shared_ptr_thread_safety();
}



// g++ -std=c++20 -lpthread -o sharedPtr_multithread.tsk sharedPtr_multithread.cpp