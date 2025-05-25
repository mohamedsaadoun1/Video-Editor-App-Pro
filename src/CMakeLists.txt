cmake_minimum_required(VERSION 3.10)
project(AdvancedVideoEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt5 packages
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core Gui Widgets Quick Qml Multimedia REQUIRED)
find_package(OpenCV REQUIRED)

# Manually specify MLT++ include directories and libraries
set(MLT_FRAMEWORK_INCLUDE_DIRS /usr/include/mlt-7/framework)
set(MLT_INCLUDE_DIRS /usr/include/mlt-7)
set(MLTxx_INCLUDE_DIRS /usr/include/mlt-7/mlt++)
set(MLT_LIBRARIES mlt-7)
set(MLTxx_LIBRARIES mlt++-7)

# Find Python for integration with rembg and whisper-cpp
find_package(Python3 COMPONENTS Interpreter Development NumPy REQUIRED)
include_directories(${Python3_INCLUDE_DIRS})

# Add executable and include source files
add_executable(AdvancedVideoEditor 
    main.cpp
    projectmanager.cpp
    projectmanager.h
    timelinemanager.cpp
    timelinemanager.h
    srt_parser.cpp
    ai_services.cpp
    ai_services.h
    audio_effects.cpp
    audio_effects.h
    beat_detector.cpp
    beat_detector.h
    background_remover.cpp
    background_remover.h
    object_tracker.cpp
    object_tracker.h
    pixabay_service.cpp
    pixabay_service.h
    advanced_transitions.cpp
    advanced_transitions.h
    utils.cpp
    utils.h
)

# Link libraries
target_link_libraries(AdvancedVideoEditor
    PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Quick
    Qt5::Qml
    Qt5::Multimedia
    ${MLTxx_LIBRARIES}
    ${MLT_LIBRARIES}
    ${OpenCV_LIBS}
    ${Python3_LIBRARIES}
)

# Include directories
target_include_directories(AdvancedVideoEditor
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MLTxx_INCLUDE_DIRS}
    ${MLT_INCLUDE_DIRS}
    ${MLT_FRAMEWORK_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${Python3_INCLUDE_DIRS}
)

# QML Resource file
qt5_add_resources(QML_RESOURCES ../qml/qml.qrc)
target_sources(AdvancedVideoEditor PRIVATE ${QML_RESOURCES})

# Copy QML files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/qml DESTINATION ${CMAKE_BINARY_DIR})

# Copy plugins to build directory
file(COPY ${CMAKE_SOURCE_DIR}/plugins DESTINATION ${CMAKE_BINARY_DIR})

# Install
install(TARGETS AdvancedVideoEditor DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/qml/ DESTINATION share/AdvancedVideoEditor/qml)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/plugins/ DESTINATION share/AdvancedVideoEditor/plugins)
