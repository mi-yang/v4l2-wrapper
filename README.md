# v4l2-wrapper
 a simple c++ wrapper of v4l2 add capture mplane support
 cross build on rk3288

# build:
 1. change cross build tool chain path in Makefile
 2. make clean && make
  

# dependences:
 pthread 

# usage:
  provide two ways to get image frame
* query

  ```javascript
  V4l2Camera webcam("/dev/video0");
  webcam.openCameraDevice(640, 480, V4L2_PIX_FMT_MJPEG);
  grabAndSaveWithTimeout(webcam,2);//grab 2 seconds and save each frame to file
  webcam.closeCameraDevice();
  ```

* callback

  ```javascript
  V4l2Camera webcam("/dev/video0");
  webcam.openCameraDevice(640, 480, V4L2_PIX_FMT_MJPEG);
  grabAndSaveWithTimeoutByCallback(webcam, 10,onFrameGrab);//grab 10 seconds and save each frame to file
  webcam.stopStreaming();
  webcam.closeCameraDevice();
  ```
