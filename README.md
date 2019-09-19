# v4l2-wrapper
 a simple c++ wrapper of v4l2

# dependences:
 pthread opencv-core opencv-highgui

# usage:
  provide two ways to get image frame
* query

  ```javascript
  V4l2Camera webcam("/dev/video0");
  webcam.openCameraDevice(640, 480, V4L2_PIX_FMT_MJPEG);
  grabAndRender(webcam, 60*5, fmt); //60*5 seconds
  webcam.closeCameraDevice();
  ```

* callback

  ```javascript
 //callback routine 
 void cb(v4l2_frame_t *frame, void *ptr)
 {
	  v4l2_frame_t *bgr = NULL;
	  V4l2Camera* pCam = (V4l2Camera*) ptr;
 }
 
  V4l2Camera webcam("/dev/video0");
  webcam.openCameraDevice(640, 480, V4L2_PIX_FMT_MJPEG);
  webcam.startStreaming(cb, &webcam);
  sleep(60*5);
  webcam.stopStreaming();
  webcam.closeCameraDevice();
  ```
