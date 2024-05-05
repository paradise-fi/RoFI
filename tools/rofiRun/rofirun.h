#ifndef ROFIRUN_H
#define ROFIRUN_H

#include <memory>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class RofiRun;
}
QT_END_NAMESPACE

class RofiRun : public QMainWindow
{
    Q_OBJECT

public:
    RofiRun(QWidget *parent = nullptr);
    ~RofiRun();

private slots:
    void loadConfiguration();

private:
    std::unique_ptr< Ui::RofiRun > ui;
};
#endif // ROFIRUN_H
