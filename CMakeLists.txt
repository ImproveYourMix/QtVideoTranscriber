cmake_minimum_required(VERSION 3.5)

project(VideoTranscriber VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

# Directory for the whisper.cpp library
add_subdirectory(whisper.cpp)

#set(TS_FILES VideoTranscriber_it_IT.ts)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    transcriber.cpp
    transcriber.h
    common.h
    common.cpp
    dr_wav.h
   # ${TS_FILES}
    transcriptionqueuemanager.h
    transcriptionqueuemanager.cpp
    transcription.h
    transcription.cpp
    transcriptionmodel.h
    transcriptionmodel.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(VideoTranscriber
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        .gitignore
    )

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(VideoTranscriber SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(VideoTranscriber
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(VideoTranscriber PRIVATE Qt${QT_VERSION_MAJOR}::Widgets whisper)

set_target_properties(VideoTranscriber PROPERTIES
    WIN32_EXECUTABLE TRUE
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/ffmpeg.exe
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E make_directory
       $<TARGET_FILE_DIR:${PROJECT_NAME}>/models
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
       ${CMAKE_SOURCE_DIR}/models/ggml-medium-q5_0.bin
       $<TARGET_FILE_DIR:${PROJECT_NAME}>/models
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_SOURCE_DIR}/models/ggml-medium.bin
      $<TARGET_FILE_DIR:${PROJECT_NAME}>/models
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
       ${CMAKE_SOURCE_DIR}/whisper.dll
       $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${Qt${QT_VERSION_MAJOR}_INSTALL_DIR}/bin/windeployqt --dir $<TARGET_FILE_DIR:${PROJECT_NAME}> $<TARGET_FILE:${PROJECT_NAME}>
)

install(TARGETS VideoTranscriber
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(VideoTranscriber)
endif()

# CPack configuration
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "VideoTranscriber")
set(CPACK_PACKAGE_VENDOR "YourName")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "VideoTranscriber Application")
set(CPACK_PACKAGE_VERSION "0.1")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "VideoTranscriber")
set(CPACK_GENERATOR "DragNDrop")
include(CPack)
