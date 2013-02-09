#include <stdio.h>
#include <stdlib.h>
#include <libfreenect.h>


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
    "DEPTH_11BIT",
    "DEPTH_10BIT",
    "DEPTH_11BIT_PACKED",
    "DEPTH_10BIT_PACKED",
    "DEPTH_REGISTERED",
    "DEPTH_MM"
};


static const char* tilt_status[] = {
    "STOPPED",
    "LIMIT",
    "MOVING"
};


static void open_camera_device(freenect_context* ctx, int device_index) 
{
    freenect_device* cam = 0;
    int video_modes = freenect_get_video_mode_count();
    int depth_modes = freenect_get_depth_mode_count();
    int i;

    freenect_select_subdevices(ctx, FREENECT_DEVICE_CAMERA);

    printf("\n");

    if(freenect_open_device(ctx, &cam, device_index) < 0) {
        fprintf(stderr, "Couldn't open camera device %d\n", device_index);
        return;
    }

    printf("Camera device on %d is opened\n", device_index);

    for(i = 0; i < video_modes; ++i) {
        freenect_frame_mode fm = freenect_get_video_mode(i);

        printf("  [video] %d x %d, %s, %d fps, %d databits per pixel, %d padding per pixel\n", 
                fm.width,
                fm.height,
                video_format[fm.video_format],
                fm.framerate,
                fm.data_bits_per_pixel,
                fm.padding_bits_per_pixel);
    }

    for(i = 0; i < depth_modes; ++i) {
        freenect_frame_mode fm = freenect_get_depth_mode(i);

        printf("  [depth] %d x %d, %s, %d fps, %d databits per pixel, %d padding per pixel\n", 
                fm.width,
                fm.height,
                depth_format[fm.depth_format],
                fm.framerate,
                fm.data_bits_per_pixel,
                fm.padding_bits_per_pixel);
    }

    freenect_close_device(cam);
}


static void open_motor_device(freenect_context* ctx, int device_index) 
{
    freenect_device* motor = 0;
    freenect_raw_tilt_state* state = 0;
    freenect_select_subdevices(ctx, FREENECT_DEVICE_MOTOR);
    double x, y, z;

    printf("\n");

    if(freenect_open_device(ctx, &motor, device_index) < 0) {
        fprintf(stderr, "Couldn't open motor device %d\n", device_index);
        return;
    }

    printf("Motor device on %d is opened\n", device_index);

    if(freenect_update_tilt_state(motor) < 0) {
        fprintf(stderr, "  Couldn't update tilt state in motor device %d\n", device_index);
        return;
    }

    state = freenect_get_tilt_state(motor);

    printf("  Tilt status: %s\n", tilt_status[freenect_get_tilt_status(state)]);
    printf("  Tilt degree: %lf\n", freenect_get_tilt_degs(state));

    freenect_get_mks_accel(state, &x, &y, &z);

    printf("  (g)   acc_x %lf, acc_y %lf, acc_z %lf\n", x, y, z);

    printf("  (raw) acc_x %d, acc_y %d, acc_z %d, tilt_encoder %d\n", 
            state->accelerometer_x,
            state->accelerometer_y,
            state->accelerometer_z,
            state->tilt_angle);

    freenect_set_led(motor, LED_YELLOW);
    freenect_set_tilt_degs(motor, 0.0);

    freenect_close_device(motor);
}



static void open_audio_device(freenect_context* ctx, int device_index)
{
    freenect_device* audio = 0;

    printf("\n");

    if(freenect_open_device(ctx, &audio, device_index) < 0) {
        fprintf(stderr, "Couldn't open motor device %d\n", device_index);
        return;
    }

    printf("Audio device on %d is opened\n", device_index);

    freenect_close_device(audio);
}





int main(int argc, char** argv)
{
    int device_index = (argc > 1) 
        ? atoi(argv[1])
        : 0;

    freenect_context* ctx;
    struct freenect_device_attributes* attributes = 0;

    if(freenect_init(&ctx, NULL) < 0) {
        fprintf(stderr, "Couldn't open kinect device %d\n", device_index);
        return EXIT_FAILURE;
    }

    freenect_set_log_level(ctx, FREENECT_LOG_DEBUG);

    int num = freenect_num_devices(ctx);
    int serials = freenect_list_device_attributes(ctx, &attributes);
    int supported = freenect_supported_subdevices();

    printf("%d kinect device(s) found on this system\n", num);

    struct freenect_device_attributes* p = attributes;
    if(serials < 0) {
        fprintf(stderr, "Couldn't list device attributes\n");
    }

    printf("%d camera serials found: ", serials);

    if(serials == 0) 
        printf("NONE");

    while(p != 0) {
        printf("%s ", p->camera_serial);
        p = p->next;
    }

    printf("\n");

    if(supported & FREENECT_DEVICE_CAMERA)
        open_camera_device(ctx, device_index);

    if(supported & FREENECT_DEVICE_MOTOR)
        open_motor_device(ctx, device_index);

    if(supported & FREENECT_DEVICE_AUDIO)
        open_audio_device(ctx, device_index);

    printf("\n");

    if(attributes != 0)
        freenect_free_device_attributes(attributes);

    freenect_shutdown(ctx);

    return EXIT_SUCCESS;
}
