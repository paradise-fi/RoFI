#include "InteractorStyle.hpp"

InteractorStyle::InteractorStyle() {
    this->lastPickedActor = nullptr;
    this->lastPickedProperty = vtkProperty::New();
    this->PartSelectedEvent = vtkCommand::UserEvent + 1;
}

void InteractorStyle::OnLeftButtonDown() {
    this->mouseMoved = false;

    // Forward events.
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}

void InteractorStyle::OnMouseMove() {
    this->mouseMoved = true;

    // Forward events.
    vtkInteractorStyleTrackballCamera::OnMouseMove();
}

void InteractorStyle::clear() { this->lastPickedActor = nullptr; }

void InteractorStyle::OnLeftButtonUp() {
    if (!this->mouseMoved) {
        // vtkRenderWindowInteractor::CreateDefaultPicker defaults to vtkPropPicker.
        vtkSmartPointer<vtkPropPicker> picker =
            vtkPropPicker::SafeDownCast(this->GetInteractor()->GetPicker());

        // We only have a single renderer.
        vtkSmartPointer<vtkRenderer> renderer = this->GetInteractor()->GetRenderWindow()
                                    ->GetRenderers()
                                    ->GetFirstRenderer();

        int position[2];
        this->GetInteractor()->GetEventPosition(position);

        picker->Pick(position[0], position[1], 0, renderer);

        // If we picked something before, reset its property to original value.
        if (this->lastPickedActor != nullptr) {
            this->lastPickedActor->GetProperty()->DeepCopy(this->lastPickedProperty);
        }

        this->lastPickedActor = picker->GetActor();
        if (this->lastPickedActor != nullptr) {
            // Save the property of the picked actor so that we can restore it next time.
            this->lastPickedProperty->DeepCopy(this->lastPickedActor->GetProperty());
            // Highlight the picked actor by changing its properties.
            this->lastPickedActor->GetProperty()->SetColor(VtkSupp::selectedColor[0], VtkSupp::selectedColor[1], VtkSupp::selectedColor[2]);
            this->lastPickedActor->GetProperty()->SetDiffuse(1.0);
            this->lastPickedActor->GetProperty()->SetSpecular(0.0);
        }

        vtkSmartPointer<vtkActor> pickedActor = picker->GetActor();
        if (pickedActor != nullptr) {
            this->InvokeEvent(this->PartSelectedEvent, pickedActor);
        }
    }

    // Forward events.
    vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
}

vtkStandardNewMacro(InteractorStyle);
