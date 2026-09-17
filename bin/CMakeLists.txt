add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE interpreter)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR})
