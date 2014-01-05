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

set(mettanode_SOURCES main.cpp PeerTableModel.cpp PeerPicker.cpp)
set(mettanode_HEADERS PeerTableModel.h PeerPicker.h)
set(mettanode_FORMS PeerPickerWindow.ui)
set(mettanode_RESOURCES ui.qrc)

if (APPLE)
    #set(MACOSX_BUNDLE_ICON_FILE "ProfileEditor.icns")
    #set_source_files_properties("img/${MACOSX_BUNDLE_ICON_FILE}" PROPERTIES
    #   MACOSX_PACKAGE_LOCATION Resources)
    #set(APPLE_BUNDLE_SRC "img/${MACOSX_BUNDLE_ICON_FILE}")
endif (APPLE)

qt4_wrap_cpp(mettanode_HEADERS_MOC ${mettanode_HEADERS})
qt4_wrap_ui(mettanode_FORMS_HEADERS ${mettanode_FORMS})
#qt4_add_resources(mettanode_RESOURCES_RCC ${pmettanode_RESOURCES})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(MettaNode ${GUI_TYPE}
    ${mettanode_SOURCES}
    ${mettanode_HEADERS_MOC}
    ${mettanode_RESOURCES_RCC}
    ${mettanode_FORMS_HEADERS}
    ${APPLE_BUNDLE_SRC})
target_link_libraries(MettaNode krypto rclient routing ssu plist support natclient nat upnpc
    ${VOICEBOX_LIBS} opus
    ${Boost_LIBRARIES} ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})
install(TARGETS MettaNode
    RUNTIME DESTINATION tools
    BUNDLE DESTINATION tools
    LIBRARY DESTINATION tools/lib
    ARCHIVE DESTINATION tools/lib)

if (APPLE)
    set_target_properties(MettaNode PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/OSXInfo.plist)
endif()
