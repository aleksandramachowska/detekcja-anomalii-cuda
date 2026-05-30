// gpu_processing.cu
#include "gpu_processing.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/sort.h>
#include <thrust/execution_policy.h>
#include <cmath>
#include <iostream>

// Komparator dla thrust, sortuje po User_id a potem po Timestamp
struct TransactionComparator {
    __host__ __device__
    bool operator()(const Transaction& a, const Transaction& b) const {
        if (a.user_id != b.user_id)
            return a.user_id < b.user_id;
        return a.timestamp < b.timestamp;
    }
};

// KERNEL CUDA: każdy wątek przetwarza 1 transakcję
__global__ void detect_anomalies_kernel(Transaction* d_data, int size, int N, float T) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx >= size) return;

    int current_user = d_data[idx].user_id;
    float current_amount = d_data[idx].amount;

    float sum = 0.0f;
    int count = 0;

    // Cofamy sie w tablicy, zeby znalezc poprzednie transakcje TEGO SAMEGO uzytkownika
    // Szukamy maksymalnie N ostatnich transakcji
    for (int i = idx; i >= 0; i--) {
        if (d_data[i].user_id != current_user) {
            break; // Inny uzytkownik, konczymy okno
        }
        
        sum += d_data[i].amount;
        count++;

        if (count == N) break; // Osiagnelismy rozmiar okna N
    }

    if (count > 0) {
        float mean = sum / count;

        // odchylenie std dla tego samego okna
        float variance_sum = 0.0f;
        int count_check = 0;
        for (int i = idx; i >= 0; i--) {
            if (d_data[i].user_id != current_user) break;
            
            float diff = d_data[i].amount - mean;
            variance_sum += diff * diff;
            count_check++;
            
            if (count_check == count) break;
        }

        // odchylenie std wg wzoru
        float std_dev = (count > 1) ? sqrtf(variance_sum / count) : 0.0f;

        // Obliczanie Z-score
        if (std_dev > 0.0f) {
            float z = (current_amount - mean) / std_dev;
            d_data[idx].z_score = z;
            if (fabsf(z) > T) {
                d_data[idx].is_anomaly = true;
            }
        } else {
            d_data[idx].z_score = 0.0f;
        }
    }
}

void run_gpu_processing(std::vector<Transaction>& chunk, int N, float T) {
    int size = chunk.size();
    Transaction* d_data = nullptr;

    // alokacja pamieci na GPU
    cudaMalloc((void**)&d_data, size * sizeof(Transaction));

    // kopiowanie danych z RAM (host) do VRAM (device)
    cudaMemcpy(d_data, chunk.data(), size * sizeof(Transaction), cudaMemcpyHostToDevice);

    // sortowanie na GPU przy uzyciu thrust - dane musza byc ułożone użytkownikami
    thrust::sort(thrust::device, d_data, d_data + size, TransactionComparator());

    // konfiguracja sieci watkow
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;

    // uruchomienie kernela
    detect_anomalies_kernel<<<blocksPerGrid, threadsPerBlock>>>(d_data, size, N, T);

    // synchronizacja i sprawdzenie bledow
    cudaDeviceSynchronize();

    // kopiowanie wynikow z powrotem do RAM
    cudaMemcpy(chunk.data(), d_data, size * sizeof(Transaction), cudaMemcpyDeviceToHost);

    // zwolnienie pamieci
    cudaFree(d_data);
}