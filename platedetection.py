from cv2 import cv2
import numpy as np
import pytesseract
pytesseract.pytesseract.tesseract_cmd = r'C:\Program Files\Tesseract-OCR\tesseract.exe'

#read the image into cv object
path = 'TruckFake.jpg'   #our image will be from video most likely jpg
og = cv2.imread(path)    #https://www.geeksforgeeks.org/python-opencv-cv2-imread-method/
og = og[1000:3000, 1000:3000]
cv2.imshow("Original", og)
cv2.waitKey()
#Efficiency Possibility: try cv2.imread(path, cv2.IMREAD_GRAYSCALE)

#Resize on standardized license plate zone (later)
#Greyscale the image
grey = cv2.cvtColor(og, cv2.COLOR_BGR2GRAY)     #https://docs.opencv.org/3.4/d8/d01/group__imgproc__color__conversions.html
# cv2.imshow("Gray-Scale", grey)
# cv2.waitKey()

#blur the image
image = cv2.bilateralFilter(grey, 5, 60, 60)      #https://docs.opencv.org/master/d4/d86/group__imgproc__filter.html
# cv2.imshow("Bilateral Filter", image)                       # Numbers determined from docs above
# cv2.waitKey()
#Efficiency Possibility: 5 is the recommended for real-time systems, but this could be reduced

#Perform edge detection
image = cv2.Canny(image, 30, 200)          #https://docs.opencv.org/3.4/da/d5c/tutorial_canny_detector.html
# cv2.imshow("Canny Edge Detection", image)
# cv2.waitKey()
#Efficiency Possibility: Write my own Canny algorithm

#Looking for contours

(contours, _) = cv2.findContours(image.copy(), cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)      #https://docs.opencv.org/master/d4/d73/tutorial_py_contours_begin.html
#contours = imutils.grab_contours(contours) Originally necessary, but removed due to this blog: https://www.pyimagesearch.com/2015/08/10/checking-your-opencv-version-using-python/
#Efficiency Possibility: Sort contours into top amount of shapes

contours = sorted(contours, key = cv2.contourArea, reverse = True)[:5]
screenCnt = None

#Find the rectangles

for c in contours:
    
    perimeter = cv2.arcLength(c, True)       #Finding perimeter (make sure shape is closed): https://opencv-python-tutroals.readthedocs.io/en/latest/py_tutorials/py_imgproc/py_contours/py_contour_features/py_contour_features.html
    approx = cv2.approxPolyDP(c, 0.015 * perimeter, True)       # Same site as above; can edit 0.018 for contour approx
 
    if len(approx) == 4:
        screenCnt = approx
        break

if screenCnt is None:
    detected = 0
    print ("No contour detected")
else:
     detected = 1

if detected == 1:
    cv2.drawContours(og, [screenCnt], -1, (0, 0, 255), 3)           #https://opencv-python-tutroals.readthedocs.io/en/latest/py_tutorials/py_imgproc/py_contours/py_contours_begin/py_contours_begin.html

# cv2.imshow("License Outline", og)
# cv2.waitKey()

#Mask the image
mask = np.zeros(grey.shape,np.uint8)                        #https://numpy.org/doc/stable/reference/generated/numpy.zeros.html
masked = cv2.drawContours(mask,[screenCnt],0,255,-1,)    
masked = cv2.bitwise_and(og,og,mask=mask)                #https://docs.opencv.org/master/d0/d86/tutorial_py_image_arithmetics.html

# cv2.imshow("Mask", masked)
# cv2.waitKey()

#Crop accordingly
(x, y) = np.where(mask == 255)                  #https://numpy.org/doc/stable/reference/generated/numpy.where.html
(topx, topy) = (np.min(x), np.min(y))         
(bottomx, bottomy) = (np.max(x), np.max(y))
cropped = grey[topx:bottomx+1, topy:bottomy+1]

cv2.imshow("Crop", cropped)
cv2.waitKey()

text = pytesseract.image_to_string(cropped, config='--psm 11')
print("Detected license plate Number is:",text)
