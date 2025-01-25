#pragma once

#include <vector>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDialog>
#include <QAbstractButton>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <QDir>
#include <memory>

namespace Ui
{
    class SelectProgram;
}

class SelectProgram : public QDialog
{
    Q_OBJECT
    
public:
    SelectProgram( QString dirPath, QWidget * parent = nullptr );
    ~SelectProgram();
    QString programPath();

signals:

private slots:
    void accept();
    void reject();
    void onClicked( QModelIndex );

private:
    QDir _directory;
    QString _programPath;
    std::unique_ptr<QStandardItemModel> _model;
    std::unique_ptr< Ui::SelectProgram > _ui;
};