from pyConfiguration import orientationToAngle, Orientation, \
    orientationToString, stringToOrientation, orientationToTransform
import numpy

orStr = orientationToTransform(Orientation.South)
print(orStr)
