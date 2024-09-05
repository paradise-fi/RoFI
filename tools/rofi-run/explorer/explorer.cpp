#include "explorer.h"
#include "./ui_explorer.h"

#include <QMessageBox>

Explorer::Explorer( std::string extension, QWidget* parent )
    : QDialog( parent ), 
      _ui( std::make_unique< Ui::Explorer >() ),
      _model( std::make_unique< QFileSystemModel >() ),
      _extension( extension )
{
    // Note: The signals and corresponding slots are connected in the .ui file
    _ui->setupUi( this );
    this->setWindowFlags( Qt::Window );
    _model->setReadOnly(true);
    _model->setRootPath(QDir::currentPath());
    _ui->treeView->setModel(_model.get());
    _ui->treeView->setRootIndex(_model->index(QDir::currentPath()));
    _ui->treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

Explorer::~Explorer() {}

QString Explorer::getPath()
{
    return _path;
}

void Explorer::clearPath()
{
    _path.clear();
}

void Explorer::reject()
{
   done(QDialog::Rejected);
}

void Explorer::accept()
{
    if ( _path.isEmpty() )
    {
        QMessageBox::critical
        (
            this,
            tr( "No file selected." ),
            tr( "You have not selected a valid file." )
        );
        return;

    }
    done(QDialog::Accepted);
}

void Explorer::setAcceptedExtension( std::string extension ) 
{
    _extension  = extension;
}

void Explorer::onClicked( QModelIndex index )
{
    QFileInfo file = _model->fileInfo( index );
    if (file.isDir())
    {
        return;
    }

    QString filePath = file.filePath();
    if ( !filePath.endsWith(QString::fromStdString( _extension ) ) )
    {
        QMessageBox::critical
        (
            this,
            tr( "Invalid file selected." ),
            tr( "You may not load a file with this format." )
        );
        return;
    }
    _ui->pathText->setText(filePath);
    _path = filePath;
}