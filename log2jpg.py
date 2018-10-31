import os
import cv2
import numpy as np
import csv
import pdb

w=1366
h=768
img = np.zeros([h,w,3],np.uint8)
count = np.zeros([h,w],np.int32)
#reading log file
print('reading log file')
with open('mouse.log',newline='') as csvfile:
    spamreader = csv.reader(csvfile, delimiter=',')
    for row in spamreader:
        # row[2:5] is in formact 'k=v',
        x = int(row[2][2:])
        y = int(row[3][2:])
        button = int(row[4][2:])
        if button>0 and x < w and y < h:
            count[y,x]+=1

mean = np.mean(count)

for i in range(0,w):
    for j in range(0,h):
        color = int((count[j,i]/mean)*255/2)
        cv2.circle(img, (i,j), 1, (color,color,color))
print('write images')
cv2.imwrite('messigray.jpg',img)
