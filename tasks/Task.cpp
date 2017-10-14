/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#define BASE_LOG_NAMESPACE Kinect

#include "Task.hpp"
#include <base-logging/Logging.hpp>
#include <string>
#include <signal.h>
#include <inttypes.h>

using namespace kinect;
using namespace base::samples;

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


bool processing_loop_running = false;


void exit_processing_loop(int signum) {
    processing_loop_running = false;
}



Task::Task(std::string const& name, RTT::base::TaskCore::TaskState state)
    : TaskBase(name, state), 
    context(NULL), 
    device(NULL),
    internal_video_buffer(NULL),
    internal_depth_buffer(NULL)
{
    log_freenect_driver_informations();

    signal(SIGUSR1, exit_processing_loop);
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine, RTT::base::TaskCore::TaskState state)
    : TaskBase(name, engine, state), 
    context(NULL), 
    device(NULL),
    internal_video_buffer(NULL),
    internal_depth_buffer(NULL)
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

        LOG_INFO("  video - %d x %d, %s, %d fps, %d databits_per_pixel, %d paddingbits_per_pixel",
                fm.width, fm.height,
                video_format[fm.video_format],
                fm.framerate,
                fm.data_bits_per_pixel,
                fm.padding_bits_per_pixel);

    }

    for(i = 0; i < depth_modes; ++i) {
        freenect_frame_mode fm = freenect_get_depth_mode(i);

        LOG_INFO("  depth - %d x %d, %s, %d fps, %d databits_per_pixel, %d paddingbits_per_pixel",
                fm.width, fm.height,
                depth_format[fm.depth_format],
                fm.framerate,
                fm.data_bits_per_pixel,
                fm.padding_bits_per_pixel);
   }

}


void Task::redirect_freenect_logs_to_rock(freenect_context* context, freenect_loglevel level, const char* msg)
{
    // remove newlines chars in msg
    std::string message(msg);

    size_t pos = message.rfind('\n');

    if(pos != std::string::npos) {
        message.replace(pos, 1, "");
    }

    switch(level) {
        case FREENECT_LOG_FATAL:
            LOG_FATAL("Freenect - %s", message.c_str());
            break;
        case FREENECT_LOG_ERROR:
            LOG_ERROR("Freenect - %s", message.c_str());
            break;
        case FREENECT_LOG_WARNING:
            LOG_WARN("Freenect - %s", message.c_str());
            break;
        case FREENECT_LOG_NOTICE:
            LOG_INFO("Freenect - %s", message.c_str());
            break;
        case FREENECT_LOG_INFO:
            LOG_INFO("Freenect - %s" ,message.c_str());
            break;
        case FREENECT_LOG_DEBUG:
        case FREENECT_LOG_SPEW:
        case FREENECT_LOG_FLOOD:
            LOG_DEBUG("Freenect - %s", message.c_str());
            break;
    }
}


bool Task::initialize_freenect(void)
{
    if(freenect_init(&context, NULL) < 0) {
        LOG_ERROR("Couldn't initialize freenect device context");
        return false;
    }

    freenect_set_log_callback(context, &Task::redirect_freenect_logs_to_rock);

    if(_freenect_flooding_log.get())
        freenect_set_log_level(context, FREENECT_LOG_FLOOD);
    else
        freenect_set_log_level(context, FREENECT_LOG_DEBUG);

    LOG_INFO("%d kinect device(s) are connected to this system", freenect_num_devices(context));
    LOG_INFO("Use kinect device with id %d", _device_id.get());

    freenect_select_subdevices(context, static_cast<freenect_device_flags>(FREENECT_DEVICE_MOTOR | FREENECT_DEVICE_CAMERA));

    if(freenect_open_device(context, &device, _device_id.get()) < 0) {
        LOG_ERROR("Couldn't open freenect devices for motor, camera and audio");
        return false;
    }

    freenect_set_user(device, this);

    freenect_set_video_callback(device, &video_capturing_callback);
    freenect_set_depth_callback(device, &depth_capturing_callback);

    return true;
}


bool Task::initialize_frames(void)
{
    // find and set frame modes
    freenect_resolution res;
    freenect_video_format format;
    
    base::samples::frame::frame_mode_t vf = _video_format.get();

    if(_video_width.get() <= 320 || _video_height.get() <= 240)
        res = FREENECT_RESOLUTION_LOW;
    else if(_video_width.get() <= 640 || _video_height.get() <= 480)
        res = FREENECT_RESOLUTION_MEDIUM;
    else
        res = FREENECT_RESOLUTION_HIGH;

    if(vf == base::samples::frame::MODE_BAYER) 
        format = FREENECT_VIDEO_BAYER;
    else if(vf == base::samples::frame::MODE_RGB)
        format = FREENECT_VIDEO_RGB;
    else if(vf == base::samples::frame::MODE_GRAYSCALE)
        format = FREENECT_VIDEO_IR_8BIT;

    video_mode = freenect_find_video_mode(res, format);
    depth_mode = freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_MM);



    if(_video_capturing.get()) {
        if(freenect_set_video_buffer(device, internal_video_buffer) < 0) {
            LOG_ERROR("Couldn't set reference to the internal video buffer in freenect");
            return false;
        }


        if(!video_mode.is_valid) {
            LOG_ERROR("Current video format configuration (%d x %d, %s) not supported", 
                    _video_width.get(), 
                    _video_height.get(),
                    (vf == base::samples::frame::MODE_BAYER ? "BAYER" :
                     (vf == base::samples::frame::MODE_RGB ? "RGB" : 
                      (vf == base::samples::frame::MODE_GRAYSCALE ? "IR" : "NONE"))));
            return false;                 
        }

        if(freenect_set_video_mode(device, video_mode) < 0) {
            LOG_ERROR("Couldn't set chosen video mode in freenect driver");
            return false;
        }

        internal_video_buffer = new uint8_t[video_mode.bytes];


    } else {
        LOG_INFO("Video capturing is disabled. No images are written to port video_frame </base/samples/frame/Frame>");
    }


    if(_depth_capturing.get()) {
	printf("Depth cpturing on\n");
        if(freenect_set_depth_buffer(device, internal_depth_buffer) < 0) {
           LOG_ERROR("Couldn't set reference to the internal depth buffer in freenect");
            return false;
        }


        if(!depth_mode.is_valid) {
            LOG_ERROR("Current depth format configuration (640 x 480, DEPTH_MM) not supported");
            return false;
        }

        if(freenect_set_depth_mode(device, depth_mode) < 0) {
            LOG_ERROR("Couldn't set chosen depth mode in freenect driver");
            return false;
        }

        DistanceImage* frame = new DistanceImage;
        internal_depth_buffer = new uint8_t[depth_mode.bytes];
        printf("Depth mode bytes: %i, width: %i, height: %i\n",depth_mode.bytes,depth_mode.width,depth_mode.height);
        frame->setSize(depth_mode.width, depth_mode.height);
        depth_frame.reset(frame);

    } else {
        LOG_INFO("Depth capturing is disabled. No images are written to port depth_frame </base/samples/DistanceImage>");
    }

    return true;
}


void kinect::video_capturing_callback(freenect_device* device, void* video, uint32_t timestamp)
{
    kinect::Task* task = reinterpret_cast<kinect::Task*>(freenect_get_user(device));

    frame::Frame* frame = new frame::Frame(task->video_mode.width, 
            task->video_mode.height, 8, task->_video_format.get()); 

    frame->setImage(reinterpret_cast<const char*>(video), task->video_mode.bytes);
    frame->setStatus(frame::STATUS_VALID);
    frame->received_time = base::Time::now();
    frame->time = base::Time::now();

    frame->attributes.clear();

    task->video_frame.reset(frame);

    task->_video_frame.write(task->video_frame);
}


void kinect::depth_capturing_callback(freenect_device* device, void* depth, uint32_t timestamp)
{
    kinect::Task* task = reinterpret_cast<kinect::Task*>(freenect_get_user(device));
   
    base::samples::DistanceImage* frame = new base::samples::DistanceImage(task->depth_mode.width,task->depth_mode.height); 

    uint16_t *data = (uint16_t*)depth;
    frame->data.resize(task->depth_mode.width*task->depth_mode.height);
    for(unsigned int i = 0; i< task->depth_mode.width*task->depth_mode.height;i++){
        frame->data[i] = data[i]/1000.0; //to meters
    }
    //From: http://vision.in.tum.de/data/datasets/rgbd-dataset/file_formats
    frame->setIntrinsic(525,525,319.5,239.5);
    frame->time = base::Time::now();

    task->depth_frame.reset(frame);

    task->_depth_image.write(task->depth_frame);
        
}


void* kinect::processing_loop(void* self) {
    kinect::Task* task = reinterpret_cast<kinect::Task*>(self);

    if(task->_video_capturing.get()) {
        if(freenect_start_video(task->device) < 0) {
            LOG_ERROR("Couldn't start video capturing");
            return false;
        } else {
            LOG_INFO("Start video capturing");
        }
    }

    if(task->_depth_capturing.get()) {
        if(freenect_start_depth(task->device) < 0) {
            LOG_ERROR("Couldn't start depth capturing");
            return false;
        } else {
            LOG_INFO("Start depth capturing");
        }
    }

    processing_loop_running = true;

    while(processing_loop_running) {
        if(freenect_process_events(task->context) < 0) {
            LOG_ERROR("Usb processor failed in update thread");
            return NULL;
        }
    }

    if(task->_video_capturing.get())
        if(freenect_stop_video(task->device) < 0)
            LOG_ERROR("Could not stop video capturing process");

    if(task->_depth_capturing.get())
        if(freenect_stop_depth(task->device) < 0)
            LOG_ERROR("Could not stop depth capturing process");

    return NULL;
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.


bool Task::startHook()
{
    
    if (! RTT::TaskContext::startHook())
        return false;

    if(!initialize_freenect() || !initialize_frames())
        return false;

    freenect_set_led(device, LED_GREEN);

    if(pthread_create(&freenect_thread, NULL, processing_loop, this)) {
        LOG_ERROR("Couldn't started usb processing thread for freenect");
        return false;
    }

     
    return true;
}



void Task::updateHook()
{
    RTT::TaskContext::updateHook();

    /*
    base::Angle angle;
    double dx, dy, dz;

    if(freenect_update_tilt_state(device) < 0) {
        LOG_ERROR("Couldn't update tilt state in motor device");
    }

    freenect_raw_tilt_state* tilt_state = freenect_get_tilt_state(device);
    freenect_tilt_status_code tilt_status = freenect_get_tilt_status(tilt_state);
    freenect_get_mks_accel(tilt_state, &dx, &dy, &dz);

    _tilt_angle.write( base::Angle::fromDeg(freenect_get_tilt_degs(tilt_state)) );

    LOG_DEBUG("Tilt acceleration: %10.3lf x %10.3lf y %10.3lf z", dx, dy, dz);

    switch(state()) {
        case TILT_MOVING:
            if(tilt_status == TILT_STATUS_STOPPED)
                state(RUNNING);
            else if(tilt_status == TILT_STATUS_LIMIT)
                state(TILT_LIMIT);
            break;

        case TILT_LIMIT:
            if(tilt_status == TILT_STATUS_STOPPED)
                state(RUNNING);
            break;

        default:
            break;
    }

    while(_tilt_command.connected() && _tilt_command.read(angle) == RTT::NewData) {
        if(freenect_set_tilt_degs(device, angle.getDeg()) <  0) {
            LOG_ERROR("Setting tilt angle with value %lf (%lf deg) failed", angle.getRad(), angle.getDeg());
        }

        state(TILT_MOVING);
    }
    */
}



void Task::errorHook()
{
    
    RTT::TaskContext::errorHook();
}



void Task::stopHook()
{
    
    RTT::TaskContext::stopHook();

    freenect_set_led(device, LED_RED);

    pthread_kill(freenect_thread, SIGUSR1);

    sleep(1);

    if(internal_video_buffer != NULL) {
        delete[] internal_video_buffer;
        internal_video_buffer = NULL;
    }

    if(internal_depth_buffer != NULL) {
        delete[] internal_depth_buffer;
        internal_depth_buffer = NULL;
    }
    
    freenect_close_device(device);
    freenect_shutdown(context);
}


