cmake_minimum_required(VERSION 3.11)

project(SufiEngine VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        master
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(raylib)

set(SOURCES
    main.cpp
)

if(ANDROID)
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

if(ANDROID)
    target_link_libraries(${PROJECT_NAME} PRIVATE log android EGL GLESv2 OpenSLES)
    
    # ==========================================
    # 🚨 الخدعة السحرية لمنع الأندرويد من طرد التطبيق 🚨
    # ==========================================
    # هذا السطر يجبر المترجم على إبقاء دالة البداية وعدم حذفها
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-u,ANativeActivity_onCreate")
endif()

if (WIN32 AND NOT MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-mwindows")
endif()
