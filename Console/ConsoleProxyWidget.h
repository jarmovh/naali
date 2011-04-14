// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_Console_ConsoleProxyWidget_h
#define incl_Console_ConsoleProxyWidget_h

#include <QGraphicsProxyWidget>

class ConsoleProxyWidget: public QGraphicsProxyWidget
{
    Q_OBJECT

public:
    explicit ConsoleProxyWidget(QWidget *widget);
    virtual ~ConsoleProxyWidget();

    //! Sets consoles relative height (0-1), 
    //! if value is not between 0-1, it will be clamped to this value range.
    void SetConsoleRelativeHeight(qreal height);

    //! Gets the consoles relative height.
    //! Value is between 0-1,0 is not visible, 1 is the whole screen.
    qreal GetConsoleRelativeHeight() const { return con_relative_height_; }

private:
    //! How much of the screen space console uses when 
    //! dropped down. 0 is not visible, 1 is the whole screen.
    qreal con_relative_height_;
};

#endif
