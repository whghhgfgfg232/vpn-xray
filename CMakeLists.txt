cmake_minimum_required(VERSION 3.21)
project(XrayQtClient VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

add_executable(XrayQtClient WIN32
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/Profile.h
    src/ProfileRepository.h
    src/ProfileRepository.cpp
    src/ProfileEditDialog.h
    src/ProfileEditDialog.cpp
    src/Subscription.h
    src/SubscriptionRepository.h
    src/SubscriptionRepository.cpp
    src/SubscriptionFetcher.h
    src/SubscriptionFetcher.cpp
    src/RunOptions.h
    src/UriParser.h
    src/UriParser.cpp
    src/XrayConfigBuilder.h
    src/XrayConfigBuilder.cpp
    src/XrayProcess.h
    src/XrayProcess.cpp
    src/XrayStatsMonitor.h
    src/XrayStatsMonitor.cpp
    src/WindowsProxyManager.h
    src/WindowsProxyManager.cpp
    src/TrafficMonitor.h
    src/TrafficMonitor.cpp
)

target_link_libraries(XrayQtClient PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

if(WIN32)
    target_link_libraries(XrayQtClient PRIVATE iphlpapi wininet)
    target_compile_definitions(XrayQtClient PRIVATE _WIN32_WINNT=0x0A00 NOMINMAX)
endif()

install(TARGETS XrayQtClient RUNTIME DESTINATION .)
