set(APP_NAME Uvvy)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Widgets finds its own dependencies.
find_package(Qt5Widgets REQUIRED)

set(gui_SOURCES
    main.cpp
    MainWindow.cpp
    QmlWindow.cpp
    AccountManager.cpp
    Contact.cpp
    Root.cpp
    UserInfo.cpp
    UserManager.cpp
    WindowManager.cpp
    Chat/ChatItemFactory.cpp
    Chat/ChatItemModel.cpp
    Chat/ChatItemDelegate.cpp
    Chat/ChatMessageItem.cpp
    Chat/ChatMessageTextEdit.cpp
    Chat/ChatTextEdit.cpp
    Chat/ChatWidget.cpp)
set(gui_FORMS
    Chat/ChatWidget.ui)

if (APPLE)
    list(APPEND gui_SOURCES ../ui/qt5/macosx.mm)
    qt5_wrap_cpp(gui_HEADERS_MOC ../ui/qt5/macsupport.h)
    set(MACOSX_BUNDLE_ICON_FILE "mettanode.icns")
    set_source_files_properties("img/${MACOSX_BUNDLE_ICON_FILE}" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    set(APPLE_BUNDLE_SRC "img/${MACOSX_BUNDLE_ICON_FILE}")
endif (APPLE)

set(gui_RESOURCES ui.qrc)
qt5_add_resources(gui_RESOURCES_RCC ${gui_RESOURCES})

add_executable(${APP_NAME} ${GUI_TYPE}
    ${gui_SOURCES}
    ${gui_HEADERS_MOC}
    ${gui_RESOURCES}
    ${gui_RESOURCES_RCC}
    ${gui_FORMS}
    ${APPLE_BUNDLE_SRC})
target_link_libraries(${APP_NAME} routing sss uia arsenal nat upnpc
    ${VOICEBOX_LIBS} opus
    ${Boost_LIBRARIES} ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})

qt5_use_modules(${APP_NAME} Widgets Quick Network)
qt5_wrap_ui(gui_FORMS_HEADERS ${gui_FORMS})

if (WIN32)
    target_link_libraries(${APP_NAME} ws2_32 gdi32 winmm)
#   RC_FILE = gui.rc
endif (WIN32)

if (APPLE)
    find_library(COREFOUNDATION_LIB CoreFoundation)
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(AUDIOTOOLBOX_LIB AudioToolbox)
    find_library(COCOA_LIB Cocoa)
    target_link_libraries(${APP_NAME}
        ${COREFOUNDATION_LIB} ${COREAUDIO_LIB} ${AUDIOTOOLBOX_LIB} ${COCOA_LIB})
    set_target_properties(${APP_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/OSXInfo.plist)
endif (APPLE)

install(TARGETS ${APP_NAME}
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

