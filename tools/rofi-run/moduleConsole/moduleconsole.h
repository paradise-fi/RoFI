#pragma once

#include <memory>
#include <QWidget>
#include <QProcess>

namespace Ui
{
class ModuleConsole;
}

class ModuleConsole : public QWidget {
    Q_OBJECT

public:
    ModuleConsole( int moduleId, bool readErr, QWidget* parent = nullptr );
    ~ModuleConsole();

    void runProgram( QString program );

signals:

private slots:
    void readProcessOut();
    void readProcessErr();

private:
    std::unique_ptr< Ui::ModuleConsole > _ui;
    std::unique_ptr< QProcess > _moduleProcess;

    int clicks = 0;
};