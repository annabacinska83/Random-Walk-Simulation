cmake_minimum_required(VERSION 3.10)
project(NahodnaPochodzkaPOS C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Pridáme priečinok s hlavičkovými súbormi
include_directories(include)

# Knižnice potrebné pre POS (vlákna a zdieľaná pamäť)
find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# --- Zatiaľ budeme kompilovať len pomocný test ---
# Keď vytvoríš server.c a main.c, pridáme ich sem.
# Teraz vytvoríme len malý test pre mapu.
add_executable(Nahodna_Pochodzka_POS src/mapa.c)

# Prepojenie s knižnicami (rt je pre zdieľanú pamäť, m pre matematiku)
target_link_libraries(Nahodna_Pochodzka_POS PRIVATE Threads::Threads rt m)