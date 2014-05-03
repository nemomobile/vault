find_package(PkgConfig REQUIRED)

pkg_check_modules(COR cor REQUIRED)
include_directories(
  ${COR_INCLUDE_DIRS}
)
link_directories(
  ${COR_LIBRARY_DIRS}
)


find_package(Qt5Core REQUIRED)
set(CMAKE_AUTOMOC TRUE)

add_library(cutespp debug.cpp os.cpp json.cpp unit.cpp sys.cpp)
qt5_use_modules(cutespp Core)
target_link_libraries(cutespp ${COR_LIBRARIES})

add_executable(vault main.cpp vault.cpp)
qt5_use_modules(vault Core)
target_link_libraries(vault cutespp libgit)
