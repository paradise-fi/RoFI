#include "Vtk.h"

std::filesystem::path getModel( const std::string& model ) {
    static ResourceFile body = LOAD_RESOURCE_FILE( model_body_obj );
    static ResourceFile shoe = LOAD_RESOURCE_FILE( model_shoe_obj );
    static ResourceFile connector = LOAD_RESOURCE_FILE( model_connector_obj );

    if ( model == "body" )
        return body.name();
    if ( model == "shoe" )
        return shoe.name();
    if ( model == "connector" )
        return connector.name();
    throw std::runtime_error( "Invalid model '" + model + "' requested" );
}


void addActor(const std::string &model, const Matrix &matrix, int color, vtkSmartPointer<vtkRenderer> &renderer)
{

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

    renderer->AddActor( frameActor );
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
            addActor("shoe", matrices[s] * shoeMatrix(), color, renderer);
            addActor("body", matrices[s] * bodyMatrix(mod.getJoint(j)), color, renderer);

            for (ConnectorId dock : {XPlus, XMinus, ZMinus})
            {
                bool on = edges[s * 3 + dock].has_value();
                double onCoeff = on ? edges[s * 3 + dock].value().onCoeff() : 0;
                addActor("connector", matrices[s] * dockMatrix(dock, on, onCoeff), color, renderer);
            }
        }
    }
}
