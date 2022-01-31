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

    Ui::ChangeColor* ui;

    QWidget* parent;

    bool showingHelp = false;

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

};

} // namespace rofi::simplesim
