## Dokumentacja Techniczna Projektu
# Autor: Aleksandra Machowska
# Informatyka II stopnia rok I niestacjoanrne
# numer albumu: 162796
# Uniwersytet Komisji Edukacji Narodowej w Krakowie

## Temat: Detekcja Anomalii w danych finansowych (mini projekt)

---

### 1. Środowisko uruchomieniowe i konfiguracja

Projekt został zaimplementowany i przetestowany w środowisku systemowym Windows 11. Do zarządzania procesem kompilacji oraz konfiguracji zależności wykorzystano narzędzie **CMake**.

* **Środowisko programistyczne IDE:** Visual Studio Code
* **Kompilator główny:** MSVC (Microsoft Visual Studio 2022 Community / MSBuild)
* **Kompilator GPU:** NVIDIA CUDA Compiler (NVCC), wersja kompatybilna z architekturą CUDA 12.x / 13.x
* **Wykorzystane biblioteki i pakiety:**
    * **Thrust (v2.0+):** wykorzystana na GPU do wysokowydajnego, równoległego sortowania struktur danych
    * **STL (Standard Template Library):** do zarządzania pamięcią na hoście, operacji wejścia/wyjścia oraz precyzyjnego pomiaru czasu

---

### 2. Architektura projektu

Kod źródłowy został podzielony modularnie:

1.  **`main.cpp`:**
    Zainicjalizowanie parametrów algorytmu ($N$ – wielkość okna, $T$ – próg Z-score),  parsowanie pliku wejściowego CSV i implementację mechanizmu **Batching / Chunking**. Dane są strumieniowane w paczkach (domyślnie po 100 000 linii), co zapobiega przepełnieniu pamięci VRAM karty graficznej (*Out-of-Core Processing*). Tutaj znajduje się też sekcja benchmarku.
2.  **`gpu_processing.cu` / `gpu_processing.h`:**
    Implementacja kernela CUDA (`detect_anomalies_kernel`). Alokacja pamięci na GPU (`cudaMalloc`), transfer danych przez PCIe (`cudaMemcpy`), sortowanie danych po `User_id` i `Timestamp` przy użyciu biblioteki Thrust, obliczenia statystyczne.
3.  **`cpu_processing.cpp` / `cpu_processing.h` (Moduł CPU):**
    Referencyjna implementacja tego samego algorytmu detekcji anomalii wykonywaną sekwencyjnie przez procesor do weryfikacji poprawności obliczeń oraz jako punkt odniesienia dla benchmarku wydajnościowego.

---

### 3. Logika algorytmu detekcji anomalii

Dla każdej transakcji algorytm wyznacza okno kroczące obejmujące maksymalnie $N$ poprzednich transakcji danego użytkownika (`User_id`). 

Wewnątrz kernela CUDA każdy wątek niezależnie przetwarza jedną transakcję, wykonując następujące kroki:

1.  cofanie się w posortowanej tablicy o maksymalnie $N$ pozycji w celu zebrania próby historycznej dla bieżącego `User_id` (pętla jest przerywana, jeśli indeks wskazuje na transakcję innego użytkownika)
2.  wyznaczenie średniej kroczącej ($\mu$) wartości transakcji w oknie
3.  obliczenie odchylenia standardowego z próby ($\sigma$) dla tego samego okna zgodnie ze wzorem:
    $$\sigma = \sqrt{\frac{\sum (x - \mu)^2}{count}}$$
4.  wyznaczenie wartości wskaźnika Z-score według wzoru:
    $$Z = \frac{x - \mu}{\sigma}$$
5.  klasyfikacja transakcji jako anomalii, jeżeli spełniony jest warunek $|Z| > T$

---

### 4. Uruchomienie - komendy

1.  **Sklonowanie repozytorium:**
    `git clone https://github.com/aleksandramachowska/detekcja-anomalii-cuda`
2.  **Wybór kompilatora - kit:**
    Skrót `Ctrl + Shift + P` + polecenie `CMake: Select a Kit` i wybierz natywny kompilator architektury x64 np. `Visual Studio Community 2022 Release - amd64`.
3.  **Generowanie plików projektu (Configure):**
    Skrót `Ctrl + Shift + P` + polecenie `CMake: Configure` - narzędzie CMake automatycznie zainicjalizuje folder `build`, wykryje obecność kompilatora `NVCC` i skonfiguruje flagi preprocesora dla MSVC.
4.  **Kompilacja:**
    `F7` - uruchomienie kompilacji kodu źródłowego C++ i kerneli `.cu`. Wynikiem budowania jest plik `anomaly_detector.exe` (zwrócony `exit code 0`).
5.  **Uruchomienie benchmarku:**
    Zaktualizuj swoją ścieżkę pliku CSV w `main.cpp`. 
    Skrót `Ctrl + Shift + P` + polecenie `CMake: Run Without Debugging`. W terminalu powinny ukazać się wyniki dla CPU i GPU.

---

### 5. Wyniki benchmarku i analiza wydajności

Testy wydajnościowe zostały przeprowadzone dla pierwszej partii danych wejściowych o rozmiarze 1000 rekordów:

* **Czas GPU:** 2.2299 s
* **Czas CPU:** 0.0000701 s ($7.01 \times 10^{-5}$ s)

---

### 6. Analiza i wnioski

Stosunek czasu, w którym kod sekwencyjny na CPU wykonał się szybciej niż kod równoległy na GPU, jest zjawiskiem przewidywalnym i wynika ze specyfikacji tej architektury:

1.  Przy pierwszym wywołaniu funkcji powiązanej z GPU sterownik NVIDIA musi zainicjalizować urządzenie, załadować kod binarny kernela oraz przydzielić wewnętrzne zasoby pamięci. Proces ten generuje ok. 2-sek narzut czasowy który obciąża pomiar wyłącznie pierwszej partii danych.
2.  Przesyłanie małych struktur danych (1000 rekordów) z pamięci operacyjnej RAM do pamięci karty graficznej VRAM i z powrotem trwa dłużej niż same obliczenia matematyczne.
3.  Procesor (CPU) posiada wyższe taktowanie pojedynczego rdzenia i mechanizmy cache przez co dobrze radzi sobie z krótkimi pętlami. Przewaga GPU jest widoczna przy przetwarzaniu dużych wolumenów danych gdzie koszt inicjalizacji kontekstu ulega amortyzacji, a tysiące rdzeni CUDA mogą w pełni zrównoleglić operacje matematyczne.