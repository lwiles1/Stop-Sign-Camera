from cv2 import cv2

car_cascade = cv2.CascadeClassifier('cars.xml')

image = cv2.imread('carWithPlate.png')
gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
 
#detects vehicle of different sizes in the input image
cars = car_cascade.detectMultiScale(gray, 1.1, 1)

for c in cars:
    print("car")