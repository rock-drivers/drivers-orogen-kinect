/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"
#include <base/logging.h>

using namespace kinect;

static const char* video_format[] = {
    "RGB",
    "BAYER",
    "IR_8BIT",
    "IR_10BIT", 
    "IR_10BIT_PACKED",
    "YUV_RGB",
    "YUV_RAW"
};


static const char* depth_format[] = {
    "11BIT", 
    "10BIT",
    "11BIT_PACKED",
    "10BIT_PACKED",
    "MM_REGISTERED",
    "MM_UNSCALED"
};


Task::Task(std::string const& name, RTT::base::TaskCore::TaskState state)
    : TaskBase(name, state), context(NULL), device(NULL)
{
    log_freenect_driver_informations();
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine, RTT::base::TaskCore::TaskState state)
    : TaskBase(name, engine, state), context(NULL), device(NULL)
{
    log_freenect_driver_informations();
}

Task::~Task()
{
}


void Task::log_freenect_driver_informations(void)
{
    int supported_devices = freenect_supported_subdevices();
    int video_modes = freenect_get_video_mode_count();
    int depth_modes = freenect_get_depth_mode_count();
    int i;

    LOG_INFO("Supported subdevices from freenect: %s%s%s%s", 
            (supported_devices & FREENECT_DEVICE_CAMERA) ? "CAMERA " : "", 
            (supported_devices & FREENECT_DEVICE_MOTOR)  ? "MOTOR " : "", 
            (supported_devices & FREENECT_DEVICE_AUDIO)  ? "AUDIO " : "", 
            !supported_devices ? "NONE" : "");

    LOG_INFO("Supported video formats from kinect camera driver:");

    for(i = 0; i < video_modes; ++i) {
        freenect_frame_mode fm = freenect_get_video_mode(i);

        RTT::log(RTT::Info) << "  video - " 
            << fm.width << " x " << fm.height << ", " 
            << video_format[fm.video_format] << ", " 
            << fm.framerate << " fps, " 
            << fm.data_bits_per_pixel << " databits per pixel, "
            << fm.padding_bits_per_pixel << " padding bits per pixel"
            << RTT::endlog();
    }

    for(i = 0; i < depth_modes; ++i) {
        freenect_frame_mode fm = freenect_get_depth_mode(i);

        RTT::log(RTT::Info) << "  depth - " 
            << fm.width << " x " << fm.height << ", " 
            << depth_format[fm.depth_format] << ", " 
            << fm.framerate << " fps, " 
            << fm.data_bits_per_pixel << " databits per pixel, "
            << fm.padding_bits_per_pixel << " padding bits per pixel"
            << RTT::endlog();
    }

}


/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.


bool Task::startHook()
{
    
    if (! RTT::TaskContext::startHook())
        return false;

    if(freenect_init(&context, NULL) < 0) {
        RTT::log(RTT::Error) << "Couldn't initialize freenect device context" << RTT::endlog();
        return false;
    }

    RTT::log(RTT::Info) << freenect_num_devices(context) << " kinect device(s) are connected to this system" << RTT::endlog();
    RTT::log(RTT::Info) << "Use kinect device with id " << _device_id.get() << RTT::endlog();

    freenect_select_subdevices(context, static_cast<freenect_device_flags>(FREENECT_DEVICE_MOTOR | FREENECT_DEVICE_CAMERA));

    if(freenect_open_device(context, &device, _device_id.get()) < 0) {
        RTT::log(RTT::Error) << "Couldn't open freenect devices for motor, camera and audio" << RTT::endlog();
        return false;
    }


    freenect_set_led(device, LED_GREEN);
       
    return true;
}



void Task::updateHook()
{
    RTT::TaskContext::updateHook();

    base::Angle angle;

    if(freenect_update_tilt_state(device) < 0) {
        RTT::log(RTT::Error) << "Couldn't update tilt state in motor device" << RTT::endlog();
    }

    freenect_raw_tilt_state* tilt_state = freenect_get_tilt_state(device);
    freenect_tilt_status_code tilt_status = freenect_get_tilt_status(tilt_state);

    _tilt_angle.write( base::Angle::fromDeg(freenect_get_tilt_degs(tilt_state)) );

    switch(state()) {
        case TILT_MOVING:
            if(tilt_status == TILT_STATUS_LIMIT || tilt_status == TILT_STATUS_STOPPED)
                state(RUNNING);
            break;
    }

    while(_tilt_command.connected() && _tilt_command.read(angle) == RTT::NewData) {
        if(freenect_set_tilt_degs(device, angle.getDeg()) <  0) {
            RTT::log(RTT::Error) << "Setting tilt angle with value " << angle.getRad() << " (" << angle.getDeg() << " deg) failed" << RTT::endlog();
        }

        state(TILT_MOVING);
    }
}



void Task::errorHook()
{
    
    RTT::TaskContext::errorHook();
}



void Task::stopHook()
{
    
    RTT::TaskContext::stopHook();

    freenect_set_led(device, LED_RED);
    
    freenect_close_device(device);
    freenect_shutdown(context);
}


