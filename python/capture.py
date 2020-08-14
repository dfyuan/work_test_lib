import cv2

capture = cv2.VideoCapture(0)

# 打开自带的摄像头
if capture.isOpened():
   width = 640
   height = 480
   # 以下两步设置显示屏的宽高
   capture .set(cv2.CAP_PROP_FRAME_WIDTH, width)
   capture .set(cv2.CAP_PROP_FRAME_HEIGHT, height)
 
   # 持续读取摄像头数据
   while True:
      read_code, frame = capture.read()
      if not read_code:
         break
      
      
      cv2.imshow("screen_title", frame)
      # 输入 q 键，保存当前画面为图片
      if cv2.waitKey(1) == ord('q'):
         # 设置图片分辨率
         frame = cv2.resize(frame, (width, height))
         cv2.imwrite('pic.png', frame)
         break
   # 释放资源      
   capture.release()
   cv2.destroyWindow("screen_title")    
