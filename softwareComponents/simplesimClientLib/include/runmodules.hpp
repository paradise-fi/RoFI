#pragma once

#include <cassert>
#include <memory>
#include <set>

#include <atoms/util.hpp>

#include <QListWidgetItem>


namespace Ui
{
class RunModules;
}

namespace rofi::simplesim
{

class RunModules : public QWidget {
    Q_OBJECT

public:
    RunModules( QWidget * parent = nullptr, std::size_t moduleCount = 0 );

    ~RunModules();
signals:

private slots:

    void accept();

private:
    std::unique_ptr< Ui::RunModules > _ui;
    std::size_t _moduleCount;
};

} // namespace rofi::simplesim
