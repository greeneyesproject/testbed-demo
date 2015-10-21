'''
Created on 06/apr/2015

@author: luca
'''

import cv2
import numpy as np
import xml.etree.ElementTree as ET
import xml.dom.minidom

STATE_NONE = 0
STATE_DRAW = 1
STATE_DELETE = 2

POINT_RADIUS = 3
POINT_COLOR_CANDIDATE = (0,127,255)
LINE_COLOR_CANDIDATE = (0,127,255)
LINE_THICK_CANDIDATE = 2
LINE_COLOR = (0,192,0)
LINE_THICK = 2

windowName = "parking"
parkingName = "deib"

def point_inside_polygon(point,poly):

    x,y=point
    n = len(poly)
    inside =False

    p1x,p1y = poly[0]
    for i in range(n+1):
        p2x,p2y = poly[i % n]
        if y > min(p1y,p2y):
            if y <= max(p1y,p2y):
                if x <= max(p1x,p2x):
                    if p1y != p2y:
                        xinters = (y-p1y)*(p2x-p1x)/(p2y-p1y)+p1x
                    if p1x == p2x or x <= xinters:
                        inside = not inside
        p1x,p1y = p2x,p2y

    return inside

def intersect(m1,a1,m2,a2):
    '''
    a1, a2 in radians
    '''
    A = np.matrix([[1, -np.tan(a1)],[1,-np.tan(a2)]])
    b = np.array([m1[1]-m1[0]*np.tan(a1),m2[1]-m2[0]*np.tan(a2)])
    x = np.linalg.solve(A,b)
    x = np.round(x,0).astype(np.int)
    return (x[1],x[0])
    
def meanAngle(point1,point2):
    '''
    Angles in [-90;90)
    '''
    mid = (np.array(point1,np.float) + np.array(point2,np.float))/2
    
    if (point1[1]-point2[1]==0):
        angle = 0
    elif (point1[0]-point2[0]==0):
        angle = -90
    else:
        angle = np.degrees(np.arctan(1.*(point1[1]-point2[1])/(point1[0]-point2[0])))
    return mid, angle

def rectify(quadr):
    
    # Determine midpoints and angular coefficients of the segments ---
    m1,a1 = meanAngle(quadr[0],quadr[1])
    m2,a2 = meanAngle(quadr[1],quadr[2])
    m3,a3 = meanAngle(quadr[2],quadr[3])
    m4,a4 = meanAngle(quadr[3],quadr[0])
    
    #  Average the angles ---
    if (np.sign(a1) == np.sign(a3)):
        avgAngle1_3 = np.mean((a1,a3))
    else:
        if np.abs(a1) > 45:
            avgAngle1_3 = np.mean(np.abs((a1,a3)))
        else:
            avgAngle1_3 = np.mean((a1,a3))
    
    if (np.sign(a2) == np.sign(a4)):
        avgAngle2_4 = np.mean((a2,a4))
    else:
        if np.abs(a2) > 45:
            avgAngle2_4 = np.mean(np.abs((a2,a4)))
        else:
            avgAngle2_4 = np.mean((a2,a4))
            
    if (avgAngle2_4 >= 0):
        avgAngle2_4 -= 90
    else:
        avgAngle2_4 += 90
    
    if (np.sign(avgAngle1_3) == np.sign(avgAngle2_4)):
        avgAngle = np.mean((avgAngle1_3,avgAngle2_4))
    else:
        if np.abs(avgAngle1_3) > 45:
            avgAngle = np.mean(np.abs((avgAngle1_3,avgAngle2_4)))
        else:
            avgAngle = np.mean((avgAngle1_3,avgAngle2_4))
    
    a1 = np.radians(avgAngle)
    a3 = a1
    a2 = avgAngle + 90
    if (a2 >= 90):
        a2 -= 180
    a2 = np.radians(a2)
    a4 = a2
    
    # Determine the intersection points between the 4 new lines ---
    p1 = intersect(m1, a1, m2, a2)
    p2 = intersect(m2, a2, m3, a3)
    p3 = intersect(m3, a3, m4, a4)
    p4 = intersect(m4, a4, m1, a1)
    
    rect = [p1,p2,p3,p4] 
    
    center = np.mean(rect,axis=0).astype(np.int)
    angle = np.floor(avgAngle-90).astype(np.int)
    w = np.linalg.norm(np.array(p1)-np.array(p2)).astype(np.int)
    h = np.linalg.norm(np.array(p2)-np.array(p3)).astype(np.int)
    
    if (w>h):
        angle +=90
        (w,h)=(h,w)
    
    rotatedRect = (center,angle,w,h)
    
    return rect,rotatedRect

def redrawImg(data):
    img = data['originalImg'].copy()
    for rect,rot_rect in zip(data['rectangles'],data['rotatedRectangles']):
        cv2.line(img,rect[0],rect[1],LINE_COLOR,LINE_THICK)
        cv2.line(img,rect[1],rect[2],LINE_COLOR,LINE_THICK)
        cv2.line(img,rect[2],rect[3],LINE_COLOR,LINE_THICK)
        cv2.line(img,rect[3],rect[0],LINE_COLOR,LINE_THICK)
        cv2.circle(img,tuple(rot_rect[0]),np.floor(0.5*min(rot_rect[2:4])).astype(np.int),LINE_COLOR,LINE_THICK);
    cv2.imshow(windowName,img)
    data['currentImg'] = img

def onMouse(event,x,y,flags,data):
    if (event == cv2.EVENT_LBUTTONUP):
        point = (x,y)
        if (data['status'] == STATE_DRAW):
            # Draw the point ---
            img = data['currentImg']
            cv2.circle(img,point,POINT_RADIUS,POINT_COLOR_CANDIDATE,-1)
            cv2.imshow(windowName,img)
            data['currentImg'] = img
            # Draw the line from the previous point, if any ---
            numPreviousPoints = len(data['candRect'])
            if numPreviousPoints > 0 and numPreviousPoints < 3:
                cv2.line(img,data['candRect'][numPreviousPoints-1],point,LINE_COLOR_CANDIDATE,LINE_THICK_CANDIDATE)
                cv2.imshow(windowName,img)
                data['currentImg'] = img
                # Add the point to the candidate rectangle ---
                data['candRect'] += [point]
            elif numPreviousPoints == 3:
                # Close the rectangle if this is the fourth point ---
                newRect = data['candRect'] + [point]
                _,newRotatedRect = rectify(newRect)
                data['rectangles'] += [newRect]
                data['rotatedRectangles'] += [newRotatedRect]
                redrawImg(data);
                data['candRect'] = []
                data['status'] = STATE_NONE
            else:
                # Add the point to the candidate rectangle ---
                data['candRect'] += [point]
        elif (data['status'] == STATE_DELETE):
            found = False;
            for idx,rect in enumerate(data['rectangles']):
                if (point_inside_polygon(point,rect)):
                    found = True
                    break
            if (found):
                del data['rectangles'][idx]
                del data['rotatedRectangles'][idx]
                redrawImg(data);
                data['status'] = STATE_NONE

def main():
    
    print('+'+'-'*10+' Parking Lot Designer v1 '+'-'*10+'+')
    print('| Press "n" to define a new parking lot'+' '*7+'|')
    print('| Press "d" to delete an existing parking lot'+' '*1+'|')
    print('| Press "w" to save the actual configuration'+' '*2+'|')
    print('| Press "q" to quit'+' '*27+'|')
    print('+'+'-'*45+'+')
    
    imgPath = "camera11.jpg"
    xmlPath = "camera11.xml"
    
    img = cv2.imread(imgPath)
    
    cv2.namedWindow(windowName)
    cv2.imshow(windowName,img)
    
    drawingStatus = {
                     "status":STATE_NONE,
                     "candRect":[],
                     "originalImg":img,
                     "currentImg":img.copy(),
                     "rectangles":[],
                     "rotatedRectangles":[],
                     }
    
    cv2.setMouseCallback(windowName,onMouse,drawingStatus)
    
    pressedKey = -1
    while (pressedKey != ord('q')):
        pressedKey = cv2.waitKey(0)
        if (pressedKey==ord('n')):
            drawingStatus['status'] = STATE_DRAW
            drawingStatus['candRect'] = []
            redrawImg(drawingStatus);
        elif(pressedKey==ord('d')):
            drawingStatus['status'] = STATE_DELETE
            drawingStatus['candRect'] = []
            redrawImg(drawingStatus);
        elif(pressedKey==ord('w')):
            print('Preparing XML')
            xmlParking = ET.Element("parking",id=parkingName)
            for idx,(rect,rotRect) in enumerate(zip(drawingStatus['rectangles'],drawingStatus['rotatedRectangles'])):
                xmlSpace = ET.SubElement(xmlParking, "space", id=str(idx+1))
                xmlRotRect = ET.SubElement(xmlSpace, "rotatedRect")
                ET.SubElement(xmlRotRect, "center", x=str(rotRect[0][0]),y=str(rotRect[0][1]))
                ET.SubElement(xmlRotRect, "size", w=str(rotRect[2]),h=str(rotRect[3]))
                ET.SubElement(xmlRotRect, "angle", d=str(rotRect[1]))
                xmlContour = ET.SubElement(xmlSpace, "contour")
                for point in rect:
                    ET.SubElement(xmlContour, "point", x=str(point[0]),y=str(point[1]))
            print('Saving to ' + xmlPath)
            xmlString = ET.tostring(xmlParking)
            xmlDom = xml.dom.minidom.parseString(xmlString)
            prettyXmlString = xmlDom.toprettyxml(indent="  ")
            fp = open(xmlPath,'w')
            fp.write(prettyXmlString)
            fp.close()
        

if __name__ == "__main__":
    main();