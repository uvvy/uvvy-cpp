set(QT_USE_QTGUI TRUE)
set(QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

set(profile_editor_SOURCES main.cpp profile_editor.cpp)
set(profile_editor_HEADERS profile_editor.h)
set(profile_editor_FORMS main_window.ui)
#set(profile_editor_RESOURCES gui.qrc)

if (APPLE)
    #set(MACOSX_BUNDLE_ICON_FILE "ProfileEditor.icns")
    #set_source_files_properties("img/${MACOSX_BUNDLE_ICON_FILE}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    #set(APPLE_BUNDLE_SRC "img/${MACOSX_BUNDLE_ICON_FILE}")
endif (APPLE)

qt4_wrap_cpp(profile_editor_HEADERS_MOC ${profile_editor_HEADERS})
qt4_wrap_ui(profile_editor_FORMS_HEADERS ${profile_editor_FORMS})
#qt4_add_resources(profile_editor_RESOURCES_RCC ${profile_editor_RESOURCES})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(ProfileEditor ${GUI_TYPE}
    ${profile_editor_SOURCES}
    ${profile_editor_HEADERS_MOC}
    ${profile_editor_RESOURCES_RCC}
    ${profile_editor_FORMS_HEADERS}
    ${APPLE_BUNDLE_SRC})
target_link_libraries(ProfileEditor krypto routing ssu plist support
    ${Boost_LIBRARIES} ${QT_LIBRARIES} ${OPENSSL_LIBRARIES})
install(TARGETS ProfileEditor
    RUNTIME DESTINATION tools
    BUNDLE DESTINATION tools
    LIBRARY DESTINATION tools/lib
    ARCHIVE DESTINATION tools/lib)

if (APPLE)
    set_target_properties(ProfileEditor PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/ProfileEditor-Info.plist)
endif()
