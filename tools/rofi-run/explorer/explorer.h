#pragma once

#include <memory>
#include <string>
#include <QDialog>
#include <QFileSystemModel>
#include <QTreeView>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class Explorer;
}
QT_END_NAMESPACE

class Explorer : public QDialog
{
    Q_OBJECT

public:
    Explorer( std::string extension, QWidget* parent = nullptr );
    ~Explorer();

    QString getPath();
    void clearPath();
    void setAcceptedExtension( std::string extension );

private slots:
    void reject();
    void accept();
    void onClicked( QModelIndex index );

private:
    std::unique_ptr< Ui::Explorer > _ui;
    std::unique_ptr< QFileSystemModel > _model;
    QString _path = "";
    std::string _extension = "";
};