# v4l2-wrapper
 a simple c++ wrapper of v4l2

# dependences:
 pthread opencv-core opencv-highgui

# usage:
  provide two ways to get image frame
* query

  `
  V4l2Camera webcam("/dev/video0");<br>
  webcam.openCameraDevice(640, 480, V4L2_PIX_FMT_MJPEG);<br>
  grabAndRender(webcam, 60*5, fmt); //60*5 seconds
  
  webcam.closeCameraDevice();
  `

* callback

  ```javascript
  V4l2Camera webcam("/dev/video0");
  webcam.openCameraDevice(640, 480, V4L2_PIX_FMT_MJPEG);
  webcam.startStreaming(cb, &webcam);
  sleep(60*5);
  webcam.stopStreaming();
  webcam.closeCameraDevice();
  ```
