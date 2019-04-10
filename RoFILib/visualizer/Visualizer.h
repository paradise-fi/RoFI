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
#include "Camera.h"
#include <sstream>

#include <vtkOBJReader.h>
#include <vtkRenderLargeImage.h>

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

Matrix shoeMatrix()
{
    return rotate(M_PI/2, X);
}

Matrix bodyMatrix(double alpha)
{
    double diff = alpha * M_PI/180.0;
    return rotate(M_PI/2 + diff, X);
}

Matrix dockMatrix(Dock dock, bool on, double onCoeff = -1)
{
    double d;
    if (onCoeff < 0){
        d = on ? 0.05 : 0;
    } else  {
        d = onCoeff * 0.05;
    }
    Matrix docks[3] = {
            translate(Vector{d,0,0}) * rotate(M_PI, Z), // Xp
            translate(Vector{-d,0,0}) * identity, // Xn
            translate(Vector{0,0,-d}) * rotate(-M_PI/2, Y) // Zn
    };
    return docks[dock];
}


class Visualizer
{
public:
    void drawConfiguration(const Configuration& config, const Camera& cameraParams) {
        drawConfiguration(config, "", false, cameraParams);
    }
    void drawConfiguration(const Configuration& config, bool savePicture, const std::string& path = ""){
        drawConfiguration(config, path, savePicture);
    }
    void drawConfiguration(const Configuration& config, const std::string& path = "", bool savePicture = false){
        Camera defaultCameraParams;
        drawConfiguration(config, path, savePicture, defaultCameraParams);
    }
    void drawConfiguration(const Configuration& config, const std::string& path, bool savePicture,
            const Camera& cameraParams);
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

void Visualizer::drawConfiguration(const Configuration &config, const std::string& path, bool savePicture,
        const Camera& cameraParams)
{
    renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renderWindow =
            vtkSmartPointer<vtkRenderWindow>::New();

    for ( const auto& [id, matrices] : config.getMatrices())
    {
        int color =  id % 7 + 3;
        const Module& mod = config.getModules().at(id);
        EdgeList edges = config.getEdges().at(id);
        for (Side s : {A, B})
        {
            Joint j = s == A ? Alpha : Beta;
            addActor("shoe", matrices[s] * shoeMatrix(), color);
            addActor("body", matrices[s] * bodyMatrix(mod.getJoint(j)), color);

            for (Dock dock : {Xp, Xn, Zn})
            {
                bool on = edges[s * 3 + dock].has_value();
                double onCoeff = on ? edges[s * 3 + dock].value().onCoeff() : 0;
                addActor("connector", matrices[s] * dockMatrix(dock, on, onCoeff), color);
            }
        }
    }

    renderer->SetBackground(1.0, 1.0, 1.0);
    renderWindow->SetSize(1920, 1080);
    renderWindow->AddRenderer(renderer);

    vtkSmartPointer<vtkCamera> camera =
            vtkSmartPointer<vtkCamera>::New();

    Vector massCenter = config.massCenter();
    if (cameraParams.defaultFoc()){
        camera->SetFocalPoint(massCenter(0), massCenter(1), massCenter(2));
    } else {
        camera->SetFocalPoint(cameraParams.getFocX(), cameraParams.getFocY(), cameraParams.getFocZ());
    }
    if (cameraParams.defaultPos()){
        camera->SetPosition(massCenter(0), massCenter(1) - 6, massCenter(2));
    } else {
        camera->SetPosition(cameraParams.getPosX(), cameraParams.getPosY(), cameraParams.getPosZ());
    }
    if (cameraParams.defaultView()){
        camera->SetViewUp(0,0,1);
    } else {
        camera->SetViewUp(cameraParams.getViewX(), cameraParams.getViewY(), cameraParams.getViewZ());
    }

    renderer->SetActiveCamera(camera);

    if (!savePicture) {
        vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
                vtkSmartPointer<vtkRenderWindowInteractor>::New();
        renderWindowInteractor->SetRenderWindow(renderWindow);

        renderWindow->Render();

        vtkSmartPointer<vtkAxesActor> axes =
                vtkSmartPointer<vtkAxesActor>::New();

        vtkSmartPointer<vtkOrientationMarkerWidget> widget =
                vtkSmartPointer<vtkOrientationMarkerWidget>::New();
        widget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
        widget->SetOrientationMarker( axes );
        widget->SetInteractor( renderWindowInteractor );
        widget->SetViewport( 0.0, 0.0, 0.4, 0.4 );
        widget->SetEnabled( 1 );
        widget->InteractiveOn();

        renderWindowInteractor->Start();

    } else {
        renderWindow->OffScreenRenderingOn();
        renderWindow->Render();

        vtkSmartPointer<vtkRenderLargeImage> renderLarge =
                vtkSmartPointer<vtkRenderLargeImage>::New();
        renderLarge->SetInput(renderer);
        renderLarge->SetMagnification(1);

        vtkSmartPointer<vtkPNGWriter> writer =
                vtkSmartPointer<vtkPNGWriter>::New();
        writer->SetFileName((path + ".png").c_str());
        writer->SetInputConnection(renderLarge->GetOutputPort());
        writer->Write();
    }
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
