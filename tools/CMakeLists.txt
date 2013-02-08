

add_executable(kinect-check-devices kinect_check_devices.cpp)

target_link_libraries(kinect-check-devices freenect)

install(TARGETS kinect-check-devices DESTINATION bin)

