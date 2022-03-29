#pragma once

#include <cassert>
#include <memory>
#include <set>

#include <atoms/util.hpp>

#include <QListWidgetItem>


namespace Ui
{
class ChangeColor;
}

namespace rofi::simplesim
{

class ChangeColor : public QWidget {
    Q_OBJECT

public:
    ChangeColor( QWidget * parent = nullptr, size_t size = 0 );

    ~ChangeColor();

    const std::vector< bool > toColor() const
    {
        return _toColor;
    }

signals:

    void pickedColor( int color );

private slots:

    void accept();

    void toggleHelp();

private:
    std::set< size_t > parseModuleIdxs( const QString & source ) const;

    std::unique_ptr< Ui::ChangeColor > _ui;

    bool _showingHelp = false;

    std::vector< bool > _toColor;
};

} // namespace rofi::simplesim
