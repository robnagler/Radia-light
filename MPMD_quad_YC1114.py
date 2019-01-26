from __future__ import print_function #Python 2.7 compatibility
import radia as rad
from uti_plot import *
import time
import numpy
import math
import csv


#print('MPMD quadrupole benchmark test 10/1')
def Material():
#Define steel MH curve
    H = [0,159.2,318.3,477.5,636.6,795.8,1591.5,3183.1,4774.6,6366.2,7957.7,15915.5,31831,47746.5,63662,79577.5,159155,318310,397887]
    M = [0,0.24,0.865,1.11,1.245,1.33,1.498,1.596,1.677,1.733,1.77,1.885,1.985,2.025,2.05,2.065,2.08,2.085,2.0851]
    convH = 4.*3.141592653589793e-07
    ma = []
    for i in range(len(H)): ma.append([H[i]*convH, M[i]])
    mp = rad.MatSatIsoTab(ma)

    return mp


#yoke
def Yoke():
    p0 = [58.527, 294.236]
    p1 = [0, 294.236]
    p2 = [0, 229]
    p3 = [35.75, 229]
    p4 = [35.75, 98]
    p5 = [17.5, 79.75]
    p6 = [17.5, 78.268]
    p9 = [78.268, 17.5]
    p10= [79.75, 17.5]
    p11= [98, 32.75]
    p12= [229, 35.75]
    p13= [229, 0]
    p14= [294.236, 0]
    p15= [294.236, 58.827]

    poly1=[p0,p1,p2,p3,p4, p5, p6]
    poly4=[p9, p10, p11, p12, p13, p14, p15]

    #Hyperbolic
    h=54

    #OC: checking reduced segmentation of the pole tip
    #nStep=21
    nStep=11

    xmin=23.2126
    xmax=h/math.sqrt(2)
    xstep=(xmax-xmin)/(nStep-1)
    ymin=xmin
    ymax=xmax
    ystep=xstep
    xlist=[]
    ylist=[]
    poly2=[]
    poly3=[]

    for i in range(nStep):
        x=xmin+i*xstep
        y=h*h/x/2
        poly2.append([x,y])
        i+=1

    for i in range(nStep):
        y=ymax-i*ystep
        x=h*h/y/2
        poly3.append([x,y])
        i+=1

    #OC
    del poly3[0]

    #OCTEST
    #print(poly1)
    #print(' ')
    #print(poly2)
    #print(' ')
    #print(poly3)
    #print(' ')
    #print(poly4)

    poly=poly1+poly2+poly3+poly4 #2D geometry

    #Triangularization
    newlist=[]
    for i in range(len(poly)):
        newlist.append([1,1])
        i+=1

    poly3D=rad.ObjMltExtTri(100,200,poly,newlist,'z',[0,0,0],'ki->Numb,TriAngMin->30,TriAreaMax->1000')

    #chamfer
    cham_y=6.7+h
    cham_ang=30/180*math.pi

    pch=[cham_y/math.sqrt(2),cham_y/math.sqrt(2),200]
    vch=[-1,-1,math.sqrt(2)*math.tan(cham_ang)]


    poly3D=rad.ObjCutMag(poly3D,pch,vch,"Frame->Lab")[0]

    rad.ObjDivMag(poly3D, [[1,1],[1,1],[5,0.2]], 'pln', [[1,0,0],[0,1,0],[0,0,1]], "Frame->LabTot")

    rad.ObjDrwAtr(poly3D, [1,1,0], 0.001)

    return poly3D


#coil
def Coil(ex):
    excitation=ex
    A=127*31.75
    j=excitation/A
    Pi=math.pi
    coil1=rad.ObjRecCur([17.875, 163.5,0],[31.75,127,400],[0,0,j])
    coil2=rad.ObjArcCur([53.75,163.5,200],[20,51.75],[-Pi/2,0],127,5,j,'man','y')
    coil3=rad.ObjArcCur([53.75,53.75,235.875],[46.25,173.25],[Pi/4,Pi/2],31.75,5,-j,'man','z')
    rad.TrfZerPerp(coil2, [0,0,0], [0,0,1])
    rad.TrfZerPerp(coil2, [0,0,0], [1,-1,0])
    rad.TrfZerPerp(coil3, [0,0,0], [0,0,1])
    rad.TrfZerPerp(coil3, [0,0,0], [1,-1,0])
    rad.TrfZerPerp(coil1, [0,0,0], [1,-1,0])

    coil=rad.ObjCnt([coil1,coil2,coil3])
    rad.ObjDrwAtr(coil, [1,0,0], 0.001)

    return coil

def CalcField(g):
  #Vertical Magnetic Field vs Horizontal Position
    xMin = 0; xMax = 50; nx = 51
    xStep = (xMax - xMin)/(nx - 1)
    yc = 0; zc = 0
    x = xMin
    Points = []
    X=[]
    for i in range(nx):
        Points.append([x,yc,zc])
        X.append(x)
        x += xStep
    ByVsX = rad.Fld(g, 'by', Points)


   #Vertical Magnetic Field vs Longitudinal Position
    zMin = -400; zMax = 400; nz = 401
    zStep = (zMax - zMin)/(nz - 1)
    xc = 40; yc = 0
    z = zMin
    Z=[]
    Points = []
    for i in range(nz):
        Points.append([xc,yc,z])
        Z.append(z)
        z += zStep
    ByVsZ = rad.Fld(g, 'by', Points)

    return ByVsX, [xMin, xMax, nx], X, ByVsZ, [zMin, zMax, nz], Z

#main
mat=Material()
yoke=Yoke()
#rad.ObjDrwOpenGL(yoke)
excitation=4832.5
rad.MatApl(yoke,mat)
coil=Coil(excitation)

#rad.ObjDrwOpenGL(coil)

rad.TrfZerPara(yoke, [0,0,0], [1,0,0])
rad.TrfZerPara(yoke, [0,0,0], [0,1,0])

rad.TrfZerPerp(yoke, [0,0,0], [0,0,1])

rad.TrfZerPara(coil, [0,0,0], [1,0,0])
rad.TrfZerPara(coil, [0,0,0], [0,1,0])
full=rad.ObjCnt([yoke,coil])

#rad.ObjDrwOpenGL(full)

t0 = time.time()
res = rad.Solve(full, 0.0001, 10000)
#print('Solved for Magnetizations in', round(time.time() - t0, 2), 's')
expect = [9.991124106723865e-05, 1.7586937018625115, 0.009296872940670615, 744.0]
assert expect == res, \
    ' {} expected != actual {}'.format(expect, res)
#print('Relaxation Results:', res)

#ByVsX, MeshX, ByVsZ, MeshZ=CalcField(coil)
#ByVsX, MeshX,xx, ByVsZ, MeshZ,zz=CalcField(full)


#csvfile = "xaxis.csv"
#with open(csvfile, "w") as output:
#    writer = csv.writer(output, lineterminator='\n')
#    writer.writerows(numpy.transpose([xx, ByVsX]))

#csvfile = "zaxis.csv"
#with open(csvfile, "w") as output:
#    writer = csv.writer(output, lineterminator='\n')
#    writer.writerows(numpy.transpose([zz, ByVsZ]))



#uti_plot1d(ByVsX, MeshX, ['Horizontal Position [mm]', 'By [T]', 'Vertical Magnetic Field'])
#uti_plot1d(ByVsZ, MeshZ, ['Longitudinal Position [mm]', 'By [T]', 'Vertical Magnetic Field at x=10mm'])
#uti_plot_show()
