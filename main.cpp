// main.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include "gpu_processing.h"
#include "cpu_processing.h"

const int CHUNK_SIZE = 100000; // rozmiar partii
const int N = 5;               // okno kroczące
const float T = 3.0f;          // próg anomalii

void process_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Nie udalo sie otworzyc pliku!" << std::endl;
        return;
    }

    std::string line;
    // pominiecie nagłówka w CSV
    std::getline(file, line); 

    std::vector<Transaction> chunk;
    int chunk_counter = 0;

    while (file.good()) {
        while (chunk.size() < CHUNK_SIZE && std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string t_id, u_id, amt, ts;
            
            std::getline(ss, t_id, ',');
            std::getline(ss, u_id, ',');
            std::getline(ss, amt, ',');
            std::getline(ss, ts, ',');

            Transaction t;
            t.transaction_id = std::stoi(t_id);
            t.user_id = std::stoi(u_id);
            t.amount = std::stof(amt);
            t.timestamp = std::stoll(ts);
            t.z_score = 0.0f;
            t.is_anomaly = false;

            chunk.push_back(t);
        }

        if (chunk.empty()) break;

        chunk_counter++;
        std::cout << "--- Przetwarzanie partii nr " << chunk_counter << " (" << chunk.size() << " rekordow) ---" << std::endl;

        // kopia danych do benchmarka CPU
        std::vector<Transaction> chunk_cpu = chunk;

        // 1. BENCHMARK GPU [s]
        auto start_gpu = std::chrono::high_resolution_clock::now();
        
        run_gpu_processing(chunk, N, T);
        
        auto end_gpu = std::chrono::high_resolution_clock::now();
        // duration na sekundy
        std::chrono::duration<double> duration_gpu = end_gpu - start_gpu;
        
        std::cout << "Czas GPU: " << duration_gpu.count() << " s" << std::endl << std::flush;

        // 2. BENCHMARK CPU [s]]
        auto start_cpu = std::chrono::high_resolution_clock::now();
        
        run_cpu_processing(chunk_cpu, N, T);
        
        auto end_cpu = std::chrono::high_resolution_clock::now();
        // duration na sekundy
        std::chrono::duration<double> duration_cpu = end_cpu - start_cpu;
        
        std::cout << "Czas CPU: " << duration_cpu.count() << " s" << std::endl << std::flush;
        
        chunk.clear();
    }
}

// dostosowanie ścieżki do pliku CSV  
int main() {
    process_file("C:\\Users\\olkam\\Desktop\\gpu-cuda\\gpu-miniprojekt\\dane_testowe.csv");
    return 0;
}