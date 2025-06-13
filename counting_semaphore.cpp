#include <iostream>
#include <thread>
#include <vector>
#include <semaphore>
#include <chrono>
#include <mutex>
#include <sstream>

// Permit up to 2 concurrent "printers"
std::counting_semaphore<2> printer_sem(2);

// Protects access to std::cout
std::mutex  io_mutex;

void safe_print(const std::string &msg) {
    std::lock_guard<std::mutex> lock(io_mutex);
    std::cout << msg;
}

void print_job(int job_id) {
    {
        std::ostringstream oss;
        oss << "Job " << job_id << " waiting for printer...\n";
        safe_print(oss.str());
    }

    printer_sem.acquire();

    {
        std::ostringstream oss;
        oss << "Job " << job_id << " started printing\n";
        safe_print(oss.str());
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    {
        std::ostringstream oss;
        oss << "Job " << job_id << " finished printing\n";
        safe_print(oss.str());
    }

    printer_sem.release();
}

int main() {
    std::vector<std::thread> jobs;
    for (int i = 1; i <= 5; ++i) {
        jobs.emplace_back(print_job, i);
    }
    for (auto &t : jobs) {
        t.join();
    }
    return 0;
}
