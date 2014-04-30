find_package(Qt5Core REQUIRED)
set(CMAKE_AUTOMOC TRUE)

add_library(cutespp debug.cpp os.cpp)
qt5_use_modules(cutespp Core)
