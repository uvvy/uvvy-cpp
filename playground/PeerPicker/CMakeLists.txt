# Qt5 - http://qt-project.org/doc/qt-5.0/qtdoc/cmake-manual.html
# Tell CMake to run moc when necessary:
#set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
#set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Widgets finds its own dependencies.
#find_package(Qt5Widgets REQUIRED)

#add_executable(PeerPicker main.cpp PeerPickerWindow.ui)

#qt5_use_modules(PeerPicker Widgets Declarative)

# Qt4
set(QT_USE_QTGUI TRUE)
set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

set(peer_picker_SOURCES main.cpp PeerInfoProvider.cpp PeerPicker.cpp)
set(peer_picker_HEADERS PeerInfoProvider.h PeerPicker.h)
set(peer_picker_FORMS PeerPickerWindow.ui)
#set(peer_picker_RESOURCES gui.qrc)

if (APPLE)
    #set(MACOSX_BUNDLE_ICON_FILE "ProfileEditor.icns")
    #set_source_files_properties("img/${MACOSX_BUNDLE_ICON_FILE}" PROPERTIES
    #   MACOSX_PACKAGE_LOCATION Resources)
    #set(APPLE_BUNDLE_SRC "img/${MACOSX_BUNDLE_ICON_FILE}")
endif (APPLE)

qt4_wrap_cpp(peer_picker_HEADERS_MOC ${peer_picker_HEADERS})
qt4_wrap_ui(peer_picker_FORMS_HEADERS ${peer_picker_FORMS})
#qt4_add_resources(peer_picker_RESOURCES_RCC ${ppeer_picker_RESOURCES})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(PeerPicker ${GUI_TYPE}
    ${peer_picker_SOURCES}
    ${peer_picker_HEADERS_MOC}
    ${peer_picker_RESOURCES_RCC}
    ${peer_picker_FORMS_HEADERS}
    ${APPLE_BUNDLE_SRC})
target_link_libraries(PeerPicker krypto rclient routing ssu plist support natclient nat upnpc
    ${Boost_LIBRARIES} ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})
install(TARGETS PeerPicker
    RUNTIME DESTINATION tools
    BUNDLE DESTINATION tools
    LIBRARY DESTINATION tools/lib
    ARCHIVE DESTINATION tools/lib)

if (APPLE)
    set_target_properties(PeerPicker PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/PeerPicker-Info.plist)
endif()
