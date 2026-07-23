cmake_minimum_required(VERSION 3.14)
project(wxhearts LANGUAGES C CXX)

# Objective-C++ (help_mac.mm) is only needed on a real Apple target.  Enable it
# *after* project() so APPLE reflects the toolchain's target platform rather
# than the host: an Android cross-build (host = macOS) must not enable OBJCXX,
# or the compiler check runs the NDK clang with macOS linker flags and fails.
if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# On Android we pull wxWidgets (the static wxQt port) flags straight from the
# kit's wx-config below; FindwxWidgets is unreliable when cross-compiling.
if(NOT ANDROID)
    find_package(wxWidgets REQUIRED COMPONENTS core base xrc html xml)

    if(NOT WIN32)
         include(${wxWidgets_USE_FILE})
    endif()
endif()

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

# ===========================================================================
#  Android (arm64-v8a) build
#
#  wxhearts is built against the *static wxWidgets Qt port* (wxQt) that ships
#  in the Qt Android kit, layered on Qt 6 Widgets/Gui/Core, and packaged into
#  an APK by androiddeployqt (driven by the `apk` target this creates).  The
#  branch is self-contained and returns early, leaving the desktop logic below
#  untouched.  It cannot reuse the desktop bin2c target because host_compile()
#  emits a plain host file (not a CMake target) when cross-compiling.
# ===========================================================================
if(ANDROID)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport)
    qt_standard_project_setup()

    # --- Host tools: bin2c (built for the host) + host wxrc ------------------
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    include(HostCompile)
    set(BIN2C ${CMAKE_BINARY_DIR}/bin2c)
    host_compile(${CMAKE_CURRENT_SOURCE_DIR}/bin2c.c ${BIN2C})

    # The kit's bin/wxrc is an Android ARM binary; force the host wxrc.
    if(NOT WXRC)
        find_program(WXRC wxrc PATHS /usr/local/bin NO_DEFAULT_PATH)
    endif()
    if(NOT WXRC)
        find_program(WXRC wxrc)
    endif()
    if(NOT WXRC)
        message(FATAL_ERROR "host wxrc not found - required to compile XRC resources")
    endif()

    # Step 1: hearts.xrc -> hearts.xrs (host wxrc)
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
                ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.png
        COMMENT "Compiling XRC resources to binary archive (Android)"
    )

    # Step 2: embed hearts.xrs as a C array via the host bin2c
    set(XRC_DATA_H ${CMAKE_CURRENT_BINARY_DIR}/xrc_data.h)
    add_custom_command(
        OUTPUT ${XRC_DATA_H}
        COMMAND ${BIN2C} ${XRC_XRS} ${XRC_DATA_H} xrc_data
        DEPENDS ${XRC_XRS} ${BIN2C}
        COMMENT "Embedding XRC archive as C array (Android)"
    )

    # --- wxWidgets (static wxQt) flags from the kit's wx-config --------------
    if(NOT wxWidgets_CONFIG_EXECUTABLE)
        set(wxWidgets_CONFIG_EXECUTABLE /usr/local/qt-6.9.3-android-arm64/bin/wx-config)
    endif()
    execute_process(COMMAND ${wxWidgets_CONFIG_EXECUTABLE} --cxxflags core base xrc html xml
        OUTPUT_VARIABLE WX_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE _wxr1)
    execute_process(COMMAND ${wxWidgets_CONFIG_EXECUTABLE} --libs core base xrc html xml
        OUTPUT_VARIABLE WX_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE _wxr2)
    if(_wxr1 OR _wxr2)
        message(FATAL_ERROR "wx-config failed: ${wxWidgets_CONFIG_EXECUTABLE}")
    endif()
    separate_arguments(WX_CXXFLAGS_LIST NATIVE_COMMAND "${WX_CXXFLAGS}")
    separate_arguments(WX_LIBS_RAW      NATIVE_COMMAND "${WX_LIBS}")
    # wx-config emits a bare library-search directory token; drop directories
    # so the linker doesn't try to open them as input files.
    set(WX_LIBS_LIST "")
    foreach(_item IN LISTS WX_LIBS_RAW)
        if(NOT IS_DIRECTORY "${_item}")
            list(APPEND WX_LIBS_LIST "${_item}")
        endif()
    endforeach()

    # --- The app: a Qt-loadable shared lib (libwxhearts_<abi>.so) ------------
    # wxIMPLEMENT_APP(HeartsApp) provides main(); QtActivity loads this .so and
    # calls it.  Widgets/Gui/Core come last so the static wx archives resolve
    # their Qt symbols against them.
    # android_evtloop_stub.cpp supplies wxGUIAppTraits::GetEventLoopSourcesManager(),
    # which the prebuilt wxQt kit references but does not define (see that file).
    qt_add_executable(wxhearts ${SOURCES} ${HEADERS} ${XRC_DATA_H}
        ${CMAKE_CURRENT_SOURCE_DIR}/android_evtloop_stub.cpp)
    target_compile_options(wxhearts PRIVATE ${WX_CXXFLAGS_LIST})
    target_include_directories(wxhearts PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
    # The static wx archives have circular references (core <-> base), so wrap
    # them in a linker group; single-pass resolution otherwise leaves symbols
    # like wxGUIAppTraits::GetEventLoopSourcesManager() undefined.
    target_link_libraries(wxhearts PRIVATE
        -Wl,--start-group ${WX_LIBS_LIST} -Wl,--end-group
        Qt6::Widgets Qt6::PrintSupport Qt6::Gui Qt6::Core)

    set_target_properties(wxhearts PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_TARGET_SDK_VERSION 35
        QT_ANDROID_MIN_SDK_VERSION 28
        QT_ANDROID_VERSION_NAME "1.0"
        QT_ANDROID_VERSION_CODE 1
    )
    return()
endif()

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
            ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.png
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

if(APPLE)
    list(APPEND SOURCES help_mac.mm)
endif()

if(WIN32)
    set(WIN_RC ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.rc)
endif()

add_executable(wxhearts WIN32 MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${XRC_DATA_H} ${ICON_FILE} ${WIN_RC})
set_target_properties(wxhearts PROPERTIES
    MACOSX_BUNDLE_ICON_FILE wxhearts.icns
    MACOSX_BUNDLE_BUNDLE_NAME "Hearts"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.mshearts.wxhearts"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
)

if(APPLE)
    target_link_libraries(wxhearts "-framework AppKit" "-framework Foundation")
endif()
target_link_libraries(wxhearts ${wxWidgets_LIBRARIES})
target_include_directories(wxhearts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
    # macOS: copy resources into bundle Resources directory
    set(BUNDLE_RESOURCES_DIR $<TARGET_FILE_DIR:wxhearts>/../Resources)

    # Copy the wxhearts.help book into Contents/Resources/ so the
    # Help menu (CFBundleHelpBookFolder / CFBundleHelpBookName in
    # Info.plist) resolves it via NSHelpManager.
    add_custom_command(TARGET wxhearts POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory
                "${BUNDLE_RESOURCES_DIR}/wxhearts.help"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.help"
                "${BUNDLE_RESOURCES_DIR}/wxhearts.help"
        COMMENT "Installing Hearts Help book into app bundle"
        VERBATIM)

elseif(UNIX)
    # Linux: define install data path and install targets
    set(WXHEARTS_DATADIR "${CMAKE_INSTALL_PREFIX}/share/wxhearts"
        CACHE PATH "Install path for wxhearts data files")
    target_compile_definitions(wxhearts PRIVATE
        WXHEARTS_DATADIR="${WXHEARTS_DATADIR}")

    install(TARGETS wxhearts RUNTIME DESTINATION bin)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.desktop      DESTINATION share/applications)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.metainfo.xml DESTINATION share/metainfo)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.6            DESTINATION share/man/man6)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons/sizes/                    DESTINATION share/icons/hicolor)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.help/Contents/Resources/en.lproj/index.html
                      ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.help/Contents/Resources/en.lproj/shared.css
                      ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.help/Contents/Resources/en.lproj/hearts_object.html
                      ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.help/Contents/Resources/en.lproj/hearts_novice.html
                      ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.help/Contents/Resources/en.lproj/hearts_score.html
                      ${CMAKE_CURRENT_SOURCE_DIR}/wxhearts.help/Contents/Resources/en.lproj/hearts_strategy.html
                      DESTINATION share/wxhearts/help)

endif()
