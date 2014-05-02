find_package(Qt5Core REQUIRED)
set(CMAKE_AUTOMOC TRUE)

add_library(cutespp debug.cpp os.cpp json.cpp)
qt5_use_modules(cutespp Core)

add_executable(vault main.cpp vault.cpp)
qt5_use_modules(vault Core)
target_link_libraries(vault cutespp libgit)
