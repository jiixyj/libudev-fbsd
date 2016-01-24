find_pkg_config(LIBEVDEV libevdev REQUIRED)

add_library(udev SHARED libudev)
target_include_directories(udev SYSTEM PRIVATE ${LIBEVDEV_INCLUDE_DIRS})
target_include_directories(udev SYSTEM PRIVATE ${LINUX_INCLUDE_DIR})

install(TARGETS udev LIBRARY DESTINATION lib)
install(FILES libudev.h DESTINATION include)

set(PKG_CONFIG_NAME libudev)
set(PKG_CONFIG_REQUIRES libevdev)
set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include")
set(PKG_CONFIG_LIBS "-L\${libdir} -ludev")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PKG_CONFIG_NAME}.pc")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PKG_CONFIG_NAME}.pc"
        DESTINATION lib/pkgconfig)
