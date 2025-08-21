add_library(copilot copilot.cpp)

target_link_libraries(copilot PRIVATE Qt6::Widgets Qt6::Network)

target_include_directories(copilot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
