// simple-multithreader.h

#ifndef SIMPLE_MULTITHREADER_H
#define SIMPLE_MULTITHREADER_H

#include <functional>
#include <pthread.h>
#include <chrono>
#include <iostream>
#include <thread>  
#include <vector>  

// ThreadData structure for 1D loop
struct ThreadData1D
{
    int low;
    int high;
    std::function<void(int)> lambda;
};

// ThreadData structure for 2D loop
struct ThreadData2D
{
    int low1;
    int high1;
    int low2;
    int high2;
    std::function<void(int, int)> lambda;
};

// Function to get the current time
auto getCurrentTime() -> std::chrono::high_resolution_clock::time_point {
    return std::chrono::high_resolution_clock::now();
}

// Function to calculate the elapsed time in microseconds
auto calculateElapsedTime(const std::chrono::high_resolution_clock::time_point& start,
                          const std::chrono::high_resolution_clock::time_point& end) -> double {
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

// Thread function for a 2D loop
void *thread_function_2d(void *arg){
    auto start_time = getCurrentTime();
    auto *data = static_cast<ThreadData2D *>(arg);

    for (int i = data->low1; i <= data->high1; ++i) {
        for (int j = data->low2; j <= data->high2; ++j) {
            data->lambda(i, j);
        }
    }

    auto end_time = getCurrentTime();
    std::cout << "Thread 2D Elapsed Time: " << calculateElapsedTime(start_time, end_time) << " us\n";

    delete data;
    return nullptr;
}

// Thread function for a 1D loop
void *thread_function_1d(void *arg){
    auto start_time = getCurrentTime();
    auto *data = static_cast<ThreadData1D *>(arg);

    for (int i = data->low; i <= data->high; ++i) {
        data->lambda(i);
    }

    auto end_time = getCurrentTime();
    std::cout << "Thread 1D Elapsed Time: " << calculateElapsedTime(start_time, end_time) << " us\n";

    delete data;
    return nullptr;
}

// parallel_for for a single-dimensional loop
void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads){
    int chunkSize = (high - low + 1) / numThreads;

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    auto threadFunction = [&lambda](int threadLow, int threadHigh) {
        auto start_time = getCurrentTime();
        for (int i = threadLow; i <= threadHigh; ++i) {
            lambda(i);
        }
        auto end_time = getCurrentTime();
        std::cout << "Thread 1D Elapsed Time: " << calculateElapsedTime(start_time, end_time) << " us\n";
    };

    for (int i = 0; i < numThreads; ++i) {
        int threadLow = low + i * chunkSize;
        int threadHigh = (i == numThreads - 1) ? high : threadLow + chunkSize - 1;

        threads.emplace_back(threadFunction, threadLow, threadHigh);
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

// parallel_for for a two-dimensional loop
void parallel_for(int low1, int high1, int low2, int high2,
                  std::function<void(int, int)> &&lambda, int numThreads){
    int chunkSize1 = (high1 - low1 + 1) / numThreads;

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    auto threadFunction = [&lambda](int threadLow1, int threadHigh1, int low2, int high2) {
        auto start_time = getCurrentTime();
        for (int i = threadLow1; i <= threadHigh1; ++i) {
            for (int j = low2; j <= high2; ++j) {
                lambda(i, j);
            }
        }
        auto end_time = getCurrentTime();
        std::cout << "Thread 2D Elapsed Time: " << calculateElapsedTime(start_time, end_time) << " us\n";
    };

    for (int i = 0; i < numThreads; ++i) {
        int threadLow1 = low1 + i * chunkSize1;
        int threadHigh1 = (i == numThreads - 1) ? high1 : threadLow1 + chunkSize1 - 1;

        threads.emplace_back(threadFunction, threadLow1, threadHigh1, low2, high2);
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

#endif // SIMPLE_MULTITHREADER_H
