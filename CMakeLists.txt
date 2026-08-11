cmake_minimum_required(VERSION 3.16)

project(ItemShopFinder VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(ItemShopFinder WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/DataStore.cpp
    src/DataStore.h
    src/OcrRunner.cpp
    src/OcrRunner.h
    src/ScreenCaptureOverlay.cpp
    src/ScreenCaptureOverlay.h
)

target_link_libraries(ItemShopFinder PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set(RAPIDOCR_RUNTIME_DIR "${CMAKE_SOURCE_DIR}/third_party/rapidocr/runtime")

if(EXISTS "${RAPIDOCR_RUNTIME_DIR}")
    add_custom_command(TARGET ItemShopFinder POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ItemShopFinder>/third_party/rapidocr"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${RAPIDOCR_RUNTIME_DIR}"
                "$<TARGET_FILE_DIR:ItemShopFinder>/third_party/rapidocr"
    )
endif()

if(MSVC)
    target_compile_options(ItemShopFinder PRIVATE /utf-8)
endif()
