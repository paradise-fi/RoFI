#include "Vtk.h"

namespace VtkSupp {
vtkInformationIntegerKey* partTypeKey = new vtkInformationIntegerKey(PART_TYPE, ROFI_KEY_LOCATION);
vtkInformationIntegerKey* moduleIdKey = new vtkInformationIntegerKey(MODULE_ID, ROFI_KEY_LOCATION);
vtkInformationIntegerKey* shoeIdKey = new vtkInformationIntegerKey(SHOE_ID, ROFI_KEY_LOCATION);
vtkInformationIntegerKey* connectorIdKey = new vtkInformationIntegerKey(CONNECTOR_ID, ROFI_KEY_LOCATION);
}

std::filesystem::path getModel(ModelPartType model) {
    static ResourceFile body = LOAD_RESOURCE_FILE( model_body_obj );
    static ResourceFile shoe = LOAD_RESOURCE_FILE( model_shoe_obj );
    static ResourceFile connector = LOAD_RESOURCE_FILE( model_connector_obj );

    switch (model) {
    case ModelPartType::BODY:
        return body.name();
    case ModelPartType::SHOE:
        return shoe.name();
    case ModelPartType::CONNECTOR:
        return connector.name();
    }
    throw std::runtime_error("Invalid model '" + std::to_string(model) + "' requested");
}

vtkSmartPointer<vtkActor> addActor(ModelPartType model, const Matrix &matrix, int id, int color, vtkSmartPointer<vtkRenderer> &renderer) {

    // vtkOBJReader is locale sensitive!
    std::locale currentLocale;
    currentLocale = std::locale::global(std::locale::classic());
    vtkSmartPointer<vtkOBJReader> reader = vtkSmartPointer<vtkOBJReader>::New();
    reader->SetFileName( getModel(model).c_str() );
    reader->Update();
    std::locale::global(currentLocale);

    vtkSmartPointer<vtkTransform> rotation = vtkSmartPointer<vtkTransform>::New();
    rotation->SetMatrix( VtkSupp::convertMatrix(matrix) );

    vtkSmartPointer<vtkTransformPolyDataFilter> filter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    filter->SetTransform( rotation );
    filter->SetInputConnection( reader->GetOutputPort() );

    vtkSmartPointer<vtkPolyDataMapper> frameMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    frameMapper->SetInputConnection(filter->GetOutputPort());

    vtkSmartPointer<vtkActor> frameActor = vtkSmartPointer<vtkActor>::New();
    frameActor->SetMapper(frameMapper);
    frameActor->GetProperty()->SetColor(VtkSupp::colors[color][0]/256.0, VtkSupp::colors[color][1]/256.0 , VtkSupp::colors[color][2]/256.0);
    frameActor->GetProperty()->SetOpacity(1.0);
    frameActor->GetProperty()->SetFrontfaceCulling(true);
    frameActor->GetProperty()->SetBackfaceCulling(true);
    frameActor->SetPosition( matrix(0,3), matrix(1,3), matrix(2,3) );
    frameActor->SetScale( 1 / 95.0 );

    // Store information about what the actor corresponds to in the configuration.
    vtkSmartPointer<vtkInformation> info = frameActor->GetPropertyKeys();
    if (!info) {
        info.TakeReference(vtkInformation::New());
        frameActor->SetPropertyKeys(info);
    }
    info->Set(VtkSupp::partTypeKey, model);
    info->Set(VtkSupp::moduleIdKey, id);

    renderer->AddActor( frameActor );

    return frameActor;
}

void VtkSupp::buildScene(Configuration* current_cfg, vtkSmartPointer<vtkRenderer> &renderer )
{
    for ( const auto& [id, matrices] : current_cfg->getMatrices())
    {
        int color =  id % 7 + 3;
        const Module& mod = current_cfg->getModules().at(id);
        EdgeList edges = current_cfg->getEdges().at(id);
        for (ShoeId s : {A, B})
        {
            Joint j = s == A ? Alpha : Beta;

            vtkSmartPointer<vtkActor> shoeActor = addActor(ModelPartType::SHOE, matrices[s] * shoeMatrix(), id, color, renderer);
            vtkSmartPointer<vtkInformation> shoeInfo = shoeActor->GetPropertyKeys();
            shoeInfo->Set(VtkSupp::shoeIdKey, s);

            vtkSmartPointer<vtkActor> bodyActor = addActor(ModelPartType::BODY, matrices[s] * bodyMatrix(mod.getJoint(j)), id, color, renderer);
            vtkSmartPointer<vtkInformation> bodyInfo = bodyActor->GetPropertyKeys();
            bodyInfo->Set(VtkSupp::shoeIdKey, s);

            for (ConnectorId dock : {XPlus, XMinus, ZMinus})
            {
                bool on = edges[s * 3 + dock].has_value();
                double onCoeff = on ? edges[s * 3 + dock].value().onCoeff() : 0;

                vtkSmartPointer<vtkActor> connectorActor = addActor(ModelPartType::CONNECTOR, matrices[s] * dockMatrix(dock, on, onCoeff), id, color, renderer);
                vtkSmartPointer<vtkInformation> connectorInfo = connectorActor->GetPropertyKeys();
                connectorInfo->Set(VtkSupp::shoeIdKey, s);
                connectorInfo->Set(VtkSupp::connectorIdKey, dock);
            }
        }
    }
}


