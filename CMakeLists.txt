cmake_minimum_required(VERSION 3.14)
project(wxhearts LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(wxWidgets REQUIRED COMPONENTS core base xrc)
include(${wxWidgets_USE_FILE})

set(SOURCES
    hearts.cpp
    main.cpp
    main2.cpp
    welcome.cpp
    card.cpp
    cards.cpp
    player.cpp
    human.cpp
    computer.cpp
    comp2.cpp
    lead.cpp
    dlg.cpp
    sound.cpp
)

set(HEADERS
    hearts.h
    main.h
    card.h
    player.h
    computer.h
    dlg.h
    resource.h
    debug.h
    regentry.h
)

# Build bin2c host tool
set(BIN2C ${CMAKE_BINARY_DIR}/bin2c)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(HostCompile)
host_compile(${CMAKE_CURRENT_SOURCE_DIR}/bin2c.c ${BIN2C})

# Find wxrc tool
find_program(WXRC wxrc)
if(NOT WXRC)
    message(FATAL_ERROR "wxrc not found - required to compile XRC resources")
endif()

# Step 1: Compile XRC to binary XRS archive (ZIP with all bmp/wav embedded)
file(GLOB BMP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/bmp/*.bmp)
set(WAV_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/glass.wav
    ${CMAKE_CURRENT_SOURCE_DIR}/timpani.wav
    ${CMAKE_CURRENT_SOURCE_DIR}/quote2.wav
)
set(XRC_XRS ${CMAKE_CURRENT_BINARY_DIR}/hearts.xrs)
add_custom_command(
    OUTPUT ${XRC_XRS}
    COMMAND ${WXRC} -o ${XRC_XRS} ${CMAKE_CURRENT_SOURCE_DIR}/hearts.xrc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hearts.xrc ${BMP_FILES} ${WAV_FILES}
    COMMENT "Compiling XRC resources to binary archive"
)

# Step 2: Embed XRS archive as C array using bin2c
set(XRC_DATA_H ${CMAKE_CURRENT_BINARY_DIR}/xrc_data.h)
add_custom_command(
    OUTPUT ${XRC_DATA_H}
    COMMAND $<TARGET_FILE:bin2c> ${XRC_XRS} ${XRC_DATA_H} xrc_data
    DEPENDS ${XRC_XRS} bin2c
    COMMENT "Embedding XRC archive as C array"
)

set(ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.icns)
set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

if(WIN32)
    set(WIN_RC ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.rc)
endif()

add_executable(wxhearts WIN32 MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${XRC_DATA_H} ${ICON_FILE} ${WIN_RC})
set_target_properties(wxhearts PROPERTIES
    MACOSX_BUNDLE_ICON_FILE wxhearts.icns
    MACOSX_BUNDLE_BUNDLE_NAME "Hearts"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.mshearts.wxhearts"
)
target_link_libraries(wxhearts ${wxWidgets_LIBRARIES})
target_include_directories(wxhearts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
    # macOS: copy resources into bundle Resources directory
    set(BUNDLE_RESOURCES_DIR $<TARGET_FILE_DIR:wxhearts>/../Resources)

elseif(UNIX)
    # Linux: define install data path and install targets
    set(WXHEARTS_DATADIR "${CMAKE_INSTALL_PREFIX}/share/wxhearts"
        CACHE PATH "Install path for wxhearts data files")
    target_compile_definitions(wxhearts PRIVATE
        WXHEARTS_DATADIR="${WXHEARTS_DATADIR}")

    install(TARGETS wxhearts RUNTIME DESTINATION bin)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.desktop      DESTINATION share/applications)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.metainfo.xml DESTINATION share/metainfo)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons/sizes/                    DESTINATION share/icons/hicolor)

endif()
