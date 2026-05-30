cmake_minimum_required(VERSION 3.18)
project(AnomalyDetection LANGUAGES CXX CUDA)

# Wymagany standard C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Wymagany standard CUDA
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Pliki źródłowe
add_executable(anomaly_detector
    main.cpp
    cpu_processing.cpp
    cpu_processing.h
    gpu_processing.cu
    gpu_processing.h
)

# Informacje o architekturze GPU
set_target_properties(anomaly_detector PROPERTIES CUDA_ARCHITECTURES "native")

# Wymuszenie nowoczesnego preprocesora dla MSVC aby CUDA 13 nie zgłaszało błędu
if(MSVC)
    target_compile_options(anomaly_detector PRIVATE 
        $<$<COMPILE_LANGUAGE:CXX>:/Zc:preprocessor>
        $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/Zc:preprocessor>
    )
endif()