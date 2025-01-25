#include "selectprogram.h"

#include <QFileInfoList>
#include <iostream>

#include "ui_selectProgram.h"

SelectProgram::SelectProgram(
    QString dirPath, QWidget* parent )
    : QDialog( parent ),
      _directory( dirPath ),
      _model( std::make_unique<QStandardItemModel>() ),
      _ui( std::make_unique<Ui::SelectProgram>() )
{
    _ui->setupUi( this );

    QFileInfoList fileInfoList = _directory.entryInfoList();

    for ( const QFileInfo &info : fileInfoList ) 
    {
        auto item = std::make_unique<QStandardItem>( info.fileName () );

        item->setData( info.absoluteFilePath(), Qt::UserRole );

        _model->appendRow( item.release() );
    }

    _ui->execList->setModel( _model.get() );
    connect( _ui->execList, SIGNAL( clicked( QModelIndex ) ), this, SLOT( onClicked( QModelIndex ) ) );
}

SelectProgram::~SelectProgram() = default;

QString SelectProgram::programPath()
{
    return _programPath;
}

void SelectProgram::onClicked( QModelIndex index )
{
    _programPath = _model->data( index, Qt::UserRole).toString();
}

void SelectProgram::accept()
{
    if ( _programPath.isEmpty() )
    {
        QMessageBox::warning
        (
            this,
            tr( "No program chosen." ),
            tr( "Please choose a valid program." )
        );
        return;
    }
    done( QDialog::Accepted );
}

void SelectProgram::reject()
{
    done( QDialog::Rejected );
}