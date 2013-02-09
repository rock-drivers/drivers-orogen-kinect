find_package(Rock)

rock_executable(kinect-check-devices
    SOURCES kinect_check_devices.cpp
    DEPS_PKGCONFIG libfreenect
    DEPS_PKGCONFIG libusb-1.0
    )
