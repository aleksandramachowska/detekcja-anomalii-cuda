// cpu_processing.cpp
#include "cpu_processing.h"
#include <algorithm>
#include <cmath>

struct CPUSort {
    bool operator()(const Transaction& a, const Transaction& b) const {
        if (a.user_id != b.user_id) return a.user_id < b.user_id;
        return a.timestamp < b.timestamp;
    }
};

void run_cpu_processing(std::vector<Transaction>& chunk, int N, float T) {
    // sortowanie jak na GPU, ale przy uzyciu CPU
    std::sort(chunk.begin(), chunk.end(), CPUSort());

    int size = chunk.size();

    // logika okna kroczacego sekwencyjnie na CPU
    for (int idx = 0; idx < size; idx++) {
        int current_user = chunk[idx].user_id;
        float current_amount = chunk[idx].amount;

        float sum = 0.0f;
        int count = 0;

        for (int i = idx; i >= 0; i--) {
            if (chunk[i].user_id != current_user) break;
            sum += chunk[i].amount;
            count++;
            if (count == N) break;
        }

        if (count > 0) {
            float mean = sum / count;
            float variance_sum = 0.0f;
            int count_check = 0;

            for (int i = idx; i >= 0; i--) {
                if (chunk[i].user_id != current_user) break;
                float diff = chunk[i].amount - mean;
                variance_sum += diff * diff;
                count_check++;
                if (count_check == count) break;
            }

            float std_dev = (count > 1) ? std::sqrt(variance_sum / count) : 0.0f;

            if (std_dev > 0.0f) {
                float z = (current_amount - mean) / std_dev;
                chunk[idx].z_score = z;
                if (std::abs(z) > T) {
                    chunk[idx].is_anomaly = true;
                }
            }
        }
    }
}