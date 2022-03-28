#pragma once

#include <cassert>
#include <memory>

#include <atoms/util.hpp>

#include <QListWidgetItem>


namespace Ui
{
class ChangeColor;
}

namespace rofi::simplesim
{

struct ChangeColor : public QWidget {
    Q_OBJECT

    std::unique_ptr< Ui::ChangeColor > ui;

    QWidget * parent;

    bool showingHelp = false;

public:
    std::vector< bool > toColor;

    ChangeColor( QWidget * parent = nullptr, size_t size = 0 );

    ~ChangeColor();

signals:

    void pickedColor( int color );

private slots:

    void accept();

    void showHelp();

private:
    void parseInput();
};

} // namespace rofi::simplesim
