
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(MyApp)
find_package(OpenCV REQUIRED)
add_executable(test_opencv test_opencv.cpp)
target_link_libraries(test_opencv PRIVATE ${OpenCV_LIBS})