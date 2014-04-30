find_package(Qt5Core REQUIRED)
set(CMAKE_AUTOMOC TRUE)

add_library(cutespp debug.cpp)
qt5_use_modules(cutespp Core)

add_executable(tmp_use_cutes test_cutes.cpp)
qt5_use_modules(tmp_use_cutes Core)
target_link_libraries(tmp_use_cutes cutespp)