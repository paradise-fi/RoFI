#pragma once

#include "Vtk.h"
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkPropPicker.h>
#include <vtkRendererCollection.h>

enum EditorTool {
    CAMERA,
    SELECTION,
};

class InteractorStyle : public vtkInteractorStyleTrackballCamera {
public:
    static InteractorStyle *New();

    int PartSelectedEvent;

    InteractorStyle();

    vtkTypeMacro(InteractorStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnLeftButtonDown() override;

    virtual void OnMouseMove() override;

    virtual void OnLeftButtonUp() override;

    /**
     * Clear actor state.
     * Needs to be called when actors are destroyed.
     */
    void clear();

private:
    bool mouseMoved = false;

    vtkSmartPointer<vtkActor> lastPickedActor = nullptr;
    vtkSmartPointer<vtkProperty> lastPickedProperty;
};
