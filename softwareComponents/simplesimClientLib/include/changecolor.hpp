#pragma once

//#include "simplesim_client.hpp"

#include <QListWidgetItem>

namespace Ui {
    class ChangeColor;
}

namespace rofi::simplesim
{

struct ChangeColor : public QWidget
{
    Q_OBJECT

public:
    std::vector< bool > to_color;

    ChangeColor( QWidget* parent = nullptr, size_t size = 0 );

    ~ChangeColor();
signals:

    void pickedColor( int color );

private slots:

    void accept();

    void showHelp();

private:

    void parseRange();

    QWidget* parent;

    Ui::ChangeColor* ui;

    bool showingHelp = false;
};

} // namespace rofi::simplesim
