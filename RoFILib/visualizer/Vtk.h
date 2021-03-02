#pragma once

// #include <vtkGenericOpenGLRenderWindow.h> // VTK 8.2 and newer

#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkProperty.h>
#include <vtkSTLReader.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkCamera.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkAxesActor.h>
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>

#include <locale>  //for locale settings, vtkObjReader si locale sensitive
#include <vtkOBJReader.h>
#include <vtkRenderLargeImage.h>

#include <string>
#include <sstream>

#include <Configuration.h>
#include <IO.h>

#include <resources.hpp>


namespace VtkSupp {

inline Matrix shoeMatrix()
{
    return rotate(M_PI/2, X);
}

inline Matrix bodyMatrix(double alpha)
{
    double diff = alpha * M_PI/180.0;
    return rotate(M_PI/2 + diff, X);
}

inline Matrix dockMatrix(ConnectorId dock, bool on, double onCoeff = -1)
{
    double d;
    if (onCoeff < 0){
        d = on ? 0.05 : 0;
    } else  {
        d = onCoeff * 0.05;
    }
    Matrix docks[3] = {
            translate(Vector{d,0,0}) * rotate(M_PI, Z), // XPlus
            translate(Vector{-d,0,0}) * identity, // XMinus
            translate(Vector{0,0,-d}) * rotate(-M_PI/2, Y) // ZMinus
    };
    return docks[dock];
}

inline vtkSmartPointer<vtkMatrix4x4> convertMatrix( const Matrix& m )
{
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
    for (int i=0; i<4; i++)
        for (int j=0; j<4; j++) {
            mat->SetElement(i,j,m(i,j));
        }
    return mat;
}


/**
 * Array of colors used to build the scene by the buildScene function.
 */
const int colors[10][3] = { {255, 255, 255},
                            {0, 255, 0},
                            {0, 0, 255},
                            {191, 218, 112},
                            {242, 202, 121},
                            {218, 152, 207},
                            {142, 202, 222},
                            {104, 135, 205},
                            {250, 176, 162},
                            {234, 110, 111}};


/**
 * @brief buildScene
 * @param current_cfg
 * @param renderer
 *
 * Function to build a scene from the given Configuration to the given vtkRenderer.
 * Colors of individual models circulate among VtkSupp:colors.
 */
void buildScene(Configuration* current_cfg, vtkSmartPointer<vtkRenderer> &renderer );
}

