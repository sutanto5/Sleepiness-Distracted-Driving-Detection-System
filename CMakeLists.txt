
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(MyApp)
find_package(OpenCV REQUIRED)
add_executable(distracted distracted.cpp)
target_link_libraries(distracted PRIVATE ${OpenCV_LIBS})