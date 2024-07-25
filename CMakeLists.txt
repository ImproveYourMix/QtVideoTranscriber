cmake_minimum_required(VERSION 3.24)

project(VideoTranscriber VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#TODO unserstand why this is needed
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

add_subdirectory(whisper.cpp)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(BUILD_SHARED_LIBS_DEFAULT OFF)
    set(MAIN_WINDOW_SOURCES
            main.cpp
            mainwindow.cpp
            mainwindow.h
            mainwindow.ui)
else()
    set(BUILD_SHARED_LIBS_DEFAULT ON)
endif()

option(BUILD_SHARED_LIBS "Build using shared libraries" ${BUILD_SHARED_LIBS_DEFAULT})

#set(TS_FILES VideoTranscriber_it_IT.ts)

set(PROJECT_SOURCES
    ${MAIN_WINDOW_SOURCES}
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
    qttranscriberwidget.h qttranscriberwidget.cpp
    qttranscriberwidget.ui
)

if(BUILD_SHARED_LIBS)
    qt_add_library(VideoTranscriber SHARED
        ${PROJECT_SOURCES}
    )
    set_target_properties(VideoTranscriber PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(VideoTranscriber PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                                       ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp/include
                                                       ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp/ggml/include)
    install(TARGETS VideoTranscriber
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
else()
    qt_add_executable(VideoTranscriber
        ${PROJECT_SOURCES}
    )
    set_target_properties(VideoTranscriber PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    install(TARGETS VideoTranscriber
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

target_link_libraries(VideoTranscriber PRIVATE Qt${QT_VERSION_MAJOR}::Widgets whisper)

if(APPLE)
    set(FFMPEG_FILE "ffmpeg")

else()
    set(FFMPEG_FILE "ffmpeg.exe")
endif()


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/${FFMPEG_FILE}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E make_directory
       $<TARGET_FILE_DIR:${PROJECT_NAME}>/models
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/models/ggml-medium.bin
      $<TARGET_FILE_DIR:${PROJECT_NAME}>/models          
    #COMMAND ${Qt${QT_VERSION_MAJOR}_INSTALL_DIR}/bin/windeployqt --dir $<TARGET_FILE_DIR:${PROJECT_NAME}> $<TARGET_FILE:${PROJECT_NAME}>
)

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
           ${CMAKE_BINARY_DIR}/bin/whisper.dll
           $<TARGET_FILE_DIR:${PROJECT_NAME}>
       COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_BINARY_DIR}/bin/ggml.dll
          $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
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
