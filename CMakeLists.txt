cmake_minimum_required(VERSION 3.14)
project(wxhearts LANGUAGES CXX)

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

set(ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.icns)
set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

if(WIN32)
    set(WIN_RC ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.rc)
endif()

add_executable(wxhearts WIN32 MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${ICON_FILE} ${WIN_RC})
set_target_properties(wxhearts PROPERTIES
    MACOSX_BUNDLE_ICON_FILE wxhearts.icns
    MACOSX_BUNDLE_BUNDLE_NAME "Hearts"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.mshearts.wxhearts"
)
target_link_libraries(wxhearts ${wxWidgets_LIBRARIES})
target_include_directories(wxhearts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(APPLE)
    # macOS: copy resources into bundle Resources directory
    set(BUNDLE_RESOURCES_DIR $<TARGET_FILE_DIR:wxhearts>/../Resources)

    add_custom_command(TARGET wxhearts POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/bmp
            ${BUNDLE_RESOURCES_DIR}/bmp
    )

    add_custom_command(TARGET wxhearts POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/hearts.xrc
            ${BUNDLE_RESOURCES_DIR}/hearts.xrc
    )

    foreach(WAV glass.wav timpani.wav quote2.wav)
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../${WAV})
            add_custom_command(TARGET wxhearts POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${CMAKE_CURRENT_SOURCE_DIR}/${WAV}
                    ${BUNDLE_RESOURCES_DIR}/${WAV}
            )
        endif()
    endforeach()

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
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bmp/
            DESTINATION share/wxhearts/bmp
            FILES_MATCHING PATTERN "*.bmp")
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/hearts.xrc
            DESTINATION share/wxhearts)
    foreach(WAV glass.wav timpani.wav quote2.wav)
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../${WAV})
            install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../${WAV}
                    DESTINATION share/wxhearts)
        endif()
    endforeach()

elseif(WIN32)
    # Windows: copy resources next to executable
    add_custom_command(TARGET wxhearts POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/bmp
            $<TARGET_FILE_DIR:wxhearts>/bmp
    )
    add_custom_command(TARGET wxhearts POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/hearts.xrc
            $<TARGET_FILE_DIR:wxhearts>/hearts.xrc
    )
    foreach(WAV glass.wav timpani.wav quote2.wav)
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../${WAV})
            add_custom_command(TARGET wxhearts POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${CMAKE_CURRENT_SOURCE_DIR}/../${WAV}
                    $<TARGET_FILE_DIR:wxhearts>/${WAV}
            )
        endif()
    endforeach()
endif()
