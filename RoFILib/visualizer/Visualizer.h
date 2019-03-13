//
// Created by xvozarov on 2/25/19.
//

#ifndef ROBOTS_VISUALIZER_H
#define ROBOTS_VISUALIZER_H

#include <vtkSphereSource.h>
#include <vtkActor.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPolyDataMapper.h>
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
#include "../Configuration.h"
#include <sstream>

#include <vtkOBJReader.h>

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

class Visualizer
{
public:
    void drawConfiguration(const Configuration& config);
private:
    void addActor(const std::string &model, const Matrix &matrix, int color) const;
    vtkSmartPointer<vtkRenderer> renderer;
};

inline vtkSmartPointer<vtkMatrix4x4> convertMatrix( const Matrix& m )
{
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
    mat->SetElement(0,0, m(0,0));
    mat->SetElement(0,1, m(0,1));
    mat->SetElement(0,2, m(0,2));
    mat->SetElement(0,3, m(0,3));
    mat->SetElement(1,0, m(1,0));
    mat->SetElement(1,1, m(1,1));
    mat->SetElement(1,2, m(1,2));
    mat->SetElement(1,3, m(1,3));
    mat->SetElement(2,0, m(2,0));
    mat->SetElement(2,1, m(2,1));
    mat->SetElement(2,2, m(2,2));
    mat->SetElement(2,3, m(2,3));
    mat->SetElement(3,0, m(3,0));
    mat->SetElement(3,1, m(3,1));
    mat->SetElement(3,2, m(3,2));
    mat->SetElement(3,3, m(3,3));
    return mat;
}

void Visualizer::drawConfiguration(const Configuration &config)
{
    renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renderWindow =
            vtkSmartPointer<vtkRenderWindow>::New();
    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
            vtkSmartPointer<vtkRenderWindowInteractor>::New();

    for ( const auto& [id, mod] : config.getModules() ) {
        int color =  id % 7 + 3;
        addActor("shoe", mod.shoeMatrix(A), color);
        addActor("shoe", mod.shoeMatrix(B), color);
        addActor("body", mod.bodyMatrix(A), color);
        addActor("body", mod.bodyMatrix(B), color);

        for (Side side : {A, B})
        {
            for (Dock dock : {Xp, Xn, Zn})
            {
                bool on = config.getEdges().at(id)[side * 3 + dock].has_value();
                addActor("connector", mod.dockMatrix(side, dock, on), color);
            }
        }

    }


    renderer->SetBackground(1.0, 1.0, 1.0);
    renderWindow->SetSize(640, 640);
    renderWindow->AddRenderer(renderer);

    vtkSmartPointer<vtkCamera> camera =
            vtkSmartPointer<vtkCamera>::New();

    Vector massCenter = config.massCenter();

    camera->SetPosition(massCenter(0), massCenter(1) - 6, massCenter(2));
    camera->SetViewUp(0,0,1);
    camera->SetFocalPoint(massCenter(0), massCenter(1), massCenter(2));

    renderer->SetActiveCamera(camera);

    vtkSmartPointer<vtkAxesActor> axes =
            vtkSmartPointer<vtkAxesActor>::New();

    renderWindowInteractor->SetRenderWindow(renderWindow);
    renderWindow->Render();

    vtkSmartPointer<vtkOrientationMarkerWidget> widget =
            vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    widget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
    widget->SetOrientationMarker( axes );
    widget->SetInteractor( renderWindowInteractor );
    widget->SetViewport( 0.0, 0.0, 0.4, 0.4 );
    widget->SetEnabled( 1 );
    widget->InteractiveOn();

    vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter =
            vtkSmartPointer<vtkWindowToImageFilter>::New();
    windowToImageFilter->SetInput(renderWindow);
//	windowToImageFilter->SetMagnification(3);
    windowToImageFilter->SetInputBufferTypeToRGBA(); //also record the alpha (transparency) channel
    windowToImageFilter->ReadFrontBufferOff(); // read from the back buffer
    windowToImageFilter->Update();

//	vtkSmartPointer<vtkPNGWriter> writer =
//			vtkSmartPointer<vtkPNGWriter>::New();
//	writer->SetFileName((path + ".png").c_str());
//	writer->SetInputConnection(windowToImageFilter->GetOutputPort());
//	writer->Write();

    renderWindowInteractor->Start();
}


void Visualizer::addActor(const std::string &model, const Matrix &matrix, int color) const
{
    std::stringstream path;
    path << "../model/" << model << ".obj";
    vtkSmartPointer<vtkOBJReader> reader =
            vtkSmartPointer<vtkOBJReader>::New();
    reader->SetFileName( path.str().c_str() );
    reader->Update();

    vtkSmartPointer<vtkTransform> rotation = vtkSmartPointer<vtkTransform>::New();
    rotation->SetMatrix( convertMatrix(matrix) );

    vtkSmartPointer<vtkTransformPolyDataFilter> filter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    filter->SetTransform( rotation );
    filter->SetInputConnection( reader->GetOutputPort() );


    vtkSmartPointer<vtkPolyDataMapper> frameMapper =
            vtkSmartPointer<vtkPolyDataMapper>::New();
    frameMapper->SetInputConnection(filter->GetOutputPort());

    vtkSmartPointer<vtkActor> frameActor =
            vtkSmartPointer<vtkActor>::New();
    frameActor->SetMapper(frameMapper);
    frameActor->SetPosition( matrix(0,3), matrix(1,3), matrix(2,3) );
    frameActor->SetScale( 1 / 95.0 );
//	frameActor->SetUserTransform( rotation );
    frameActor->GetProperty()->SetColor(colors[color][0]/256.0, colors[color][1]/256.0 , colors[color][2]/256.0);

    renderer->AddActor(frameActor);
}

#endif //ROBOTS_VISUALIZER_H
