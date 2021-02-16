from cv2 import cv2
import imutils

#read the image into cv object
path = 'carWithPlate.png'   #our image will be from video most likely jpg
image = cv2.imread(path)    #https://www.geeksforgeeks.org/python-opencv-cv2-imread-method/
cv2.imshow("Original", image)
cv2.waitKey()
#Efficiency Possibility: try cv2.imread(path, cv2.IMREAD_GRAYSCALE)

#Resize on standardized license plate zone (later)
#Greyscale the image
image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)     #https://docs.opencv.org/3.4/d8/d01/group__imgproc__color__conversions.html
cv2.imshow("Gray-Scale", image)
cv2.waitKey()

#blur the image
image = cv2.bilateralFilter(image, 5, 60, 60)      #https://docs.opencv.org/master/d4/d86/group__imgproc__filter.html
cv2.imshow("Bilateral Filter", image)                       # Numbers determined from docs above
cv2.waitKey()
#Efficiency Possibility: 5 is the recommended for real-time systems, but this could be reduced

#Perform edge detection
image = cv2.Canny(image, 100, 200)          #https://docs.opencv.org/3.4/da/d5c/tutorial_canny_detector.html
cv2.imshow("Canny Edge Detection", image)
cv2.waitKey()
#Efficiency Possibility: Write my own Canny algorithm

#Looking for contours