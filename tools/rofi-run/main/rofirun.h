#ifndef ROFIRUN_H
#define ROFIRUN_H

#include <memory>
#include "../explorer/explorer.h"
#include <QAbstractButton>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui 
{
    class RofiRun;
}
QT_END_NAMESPACE

class RofiRun : public QMainWindow
{
    Q_OBJECT

public:
    RofiRun( QWidget *parent = nullptr );
    ~RofiRun();

private slots:
    void loadConfiguration();
    void selectConfiguration();
    void selectFilter( QAbstractButton* button );

private:
    QString _config_path;
    QString _filter_path;
    std::unique_ptr< Ui::RofiRun > ui;
    std::unique_ptr< Explorer > _explorer;

    void setUpRadioButton();
    void openExplorer( QString& result, std::string acceptedExtension );
};
#endif // ROFIRUN_H
