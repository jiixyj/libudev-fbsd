include(CheckIncludeFile)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVDEV IMPORTED_TARGET libevdev REQUIRED)
check_include_file("linux/input.h" HAVE_LINUX_INPUT_H)


add_library(udev SHARED libudev)
target_link_libraries(udev PRIVATE PkgConfig::LIBEVDEV)
target_link_libraries(udev PRIVATE Threads::Threads)
if(NOT HAVE_LINUX_INPUT_H)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message(WARNING "Running Linux, but linux/input.h not found. Are you sure this is correct?")
  endif()
  target_include_directories(udev SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
endif()

set_target_properties(udev PROPERTIES SOVERSION 1)

add_executable(udev-test udev_test)
target_link_libraries(udev-test udev)

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
