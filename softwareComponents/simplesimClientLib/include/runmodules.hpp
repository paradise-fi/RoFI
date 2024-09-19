#pragma once

#include <cassert>
#include <memory>
#include <set>

#include <atoms/util.hpp>

#include <QListWidgetItem>
#include <QDialog>


namespace Ui
{
class RunModules;
}

namespace rofi::simplesim
{

class RunModules : public QDialog {
    Q_OBJECT

public:
    RunModules( QWidget * parent = nullptr );

    ~RunModules();

    std::string getProgram();
    
signals:

private slots:

    void accept();

private:
    std::unique_ptr< Ui::RunModules > _ui;
    std::string _program;
};

} // namespace rofi::simplesim
