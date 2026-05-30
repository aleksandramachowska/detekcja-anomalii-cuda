// gpu_processing.h
#pragma once
#include <vector>

struct Transaction {
    int transaction_id;
    int user_id;
    float amount;
    long long timestamp;
    float z_score;
    bool is_anomaly;
};

// funkcja do main.cpp
void run_gpu_processing(std::vector<Transaction>& chunk, int N, float T);