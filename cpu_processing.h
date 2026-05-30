// cpu_processing.h
#pragma once
#include "gpu_processing.h"
#include <vector>

void run_cpu_processing(std::vector<Transaction>& chunk, int N, float T);