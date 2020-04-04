#include "rofiapp_mainWindow.h"
#include <QApplication>


#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Rofiapp_MainWindow w;


// widget.resize( 256, 256 );

  vtkSmartPointer<vtkSphereSource> sphereSource =
      vtkSmartPointer<vtkSphereSource>::New();

  vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  sphereMapper->SetInputConnection( sphereSource->GetOutputPort() );

  vtkSmartPointer<vtkActor> sphereActor =
      vtkSmartPointer<vtkActor>::New();
  sphereActor->SetMapper( sphereMapper );

  vtkSmartPointer<vtkRenderer> renderer =
      vtkSmartPointer<vtkRenderer>::New();
  renderer->AddActor( sphereActor );
 
//  qvtkWidget.GetRenderWindow()->AddRenderer( renderer );
  //widget.show();



    w.show();

    return a.exec();
}
