require "orocos.rb"

include Orocos
Orocos.initialize

Orocos.run "kinect::Task" => "kinect" do
  kinect = TaskContext.get "kinect"
  angle = Types::Base::Angle.new
  tilt_writer = kinect.tilt_command.writer
  kinect.video_format = :MODE_RGB
#  kinect.freenect_flooding_log = true
#  kinect.video_format = :MODE_GRAYSCALE
  kinect.start

  angle.rad = 30 * (Math::PI / 180.0)
  tilt_writer.write angle

  sleep 8

  angle.rad = -30 * (Math::PI / 180.0)
  tilt_writer.write angle

  sleep 8
 
  angle.rad = 0
  tilt_writer.write angle

  sleep 8
  loop do
	sleep 1
  end

  kinect.stop
end
