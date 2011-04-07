// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "UiConsoleManager.h"
#include "ui_ConsoleWidget.h"
#include "ConsoleProxyWidget.h"
#include "ConsoleEvents.h"
#include "ConsoleManager.h"
#include "ConsoleModule.h"

#include "UiAPI.h"
#include "UiProxyWidget.h"
#include "EventManager.h"
#include "Framework.h"

#include <QGraphicsView>
#include <QString>
#include <QRegExp>
#include <QRectF>

#include "MemoryLeakCheck.h"

namespace Console
{
    UiConsoleManager::UiConsoleManager(Foundation::Framework *framework, QGraphicsView *ui_view) :
        framework_(framework),
        ui_view_(ui_view),
        console_ui_(0),
        console_widget_(0),
        proxy_widget_(0),
        visible_(false),
        opacity_(0.8),
        hooked_to_scenes_(false)
    {
        if (!ui_view_)
            return; // Headless

        console_ui_ = new Ui::ConsoleWidget();
        console_widget_ = new QWidget();

        // Init internals
        console_ui_->setupUi(console_widget_);
        ///\todo UiService deprecated.
        /*
        UiServicePtr ui = framework_->GetService<UiServiceInterface>(Service::ST_Gui).lock();
        if (ui)
        {
            proxy_widget_ = ui->AddWidgetToScene(console_widget_);
            proxy_widget_->setMinimumHeight(0);
            proxy_widget_->setGeometry(QRect(0, 0, ui_view_->width(), 0));
            proxy_widget_->setOpacity(opacity_);
            proxy_widget_->setZValue(100);
            ui->RegisterUniversalWidget("Console", proxy_widget_);
        }
        */

        if (framework_->Ui())
        {
            proxy_widget_ = framework_->Ui()->AddWidgetToScene(console_widget_);
            proxy_widget_->setMinimumHeight(0);
            proxy_widget_->setGeometry(QRect(0, 0, ui_view_->width(), 0));
            proxy_widget_->setOpacity(opacity_);
            proxy_widget_->setZValue(100);
        }

        // Init animation
        animation_.setTargetObject(proxy_widget_);
        animation_.setPropertyName("geometry");
        animation_.setDuration(300);

        // Handle line edit input
        connect(console_ui_->ConsoleInputArea, SIGNAL(returnPressed()), SLOT(HandleInput()));
        // Print queuing with Qt::QueuedConnection to avoid problems when printing from threads
        connect(this, SIGNAL(PrintOrderReceived(const QString &)), SLOT(PrintToConsole(const QString &)), Qt::QueuedConnection);
        // Init event categories
        console_category_id_ = framework_->GetEventManager()->QueryEventCategory("Console");
    }

    UiConsoleManager::~UiConsoleManager()
    {
        // console_widget_ gets deleted by the scene it is in currently
        SAFE_DELETE(console_ui_);
    }

    void UiConsoleManager::HandleInput()
    {
        if (!console_ui_)
            return; // Headless
        QString text = console_ui_->ConsoleInputArea->text();
        Console::ConsoleEventData event_data(text.toStdString());
        framework_->GetEventManager()->SendEvent(console_category_id_, Console::Events::EVENT_CONSOLE_COMMAND_ISSUED, &event_data);
        console_ui_->ConsoleInputArea->clear();
    }

    void UiConsoleManager::QueuePrintRequest(const QString &text)
    {
        if (!console_ui_)
            return; // Headless
        emit PrintOrderReceived(text);
    }

    void UiConsoleManager::PrintToConsole(const QString &text)
    {
        if (!console_ui_)
            return; // Headless
        QString html = Qt::escape(text);
        StyleString(html);
        console_ui_->ConsoleTextArea->appendHtml(html);
    }

    void UiConsoleManager::AdjustToSceneRect(const QRectF& rect)
    {
        if (visible_)
        {
            QRectF new_size = rect;
            new_size.setHeight(rect.height() * 0.5);
            proxy_widget_->setGeometry(new_size);
        }
        else
        {
            proxy_widget_->hide();
        }
    }

    void UiConsoleManager::KeyPressed(KeyEvent *key_event)
    {
        if (key_event->keyCode == Qt::Key_F1)
            ToggleConsole();
    }

    void UiConsoleManager::ToggleConsole()
    {
        if (!ui_view_)
            return;

        if (!hooked_to_scenes_)
        {
            QGraphicsScene *scene = framework_->Ui()->GraphicsScene();
            if (scene)
                connect(scene, SIGNAL( sceneRectChanged(const QRectF &)), SLOT( AdjustToSceneRect(const QRectF &) ));
            hooked_to_scenes_ = true;
            ///\todo UiService is deprecated.
            /*
            UiServicePtr ui = framework_->GetService<UiServiceInterface>(Service::ST_Gui).lock();
            if (ui)
            {
                QGraphicsScene *scene = ui->GetScene("Inworld");
                if (scene)
                    connect(scene, SIGNAL( sceneRectChanged(const QRectF &)), SLOT( AdjustToSceneRect(const QRectF &) ));
                scene = ui->GetScene("WorldBuilding");
                if (scene)
                    connect(scene, SIGNAL( sceneRectChanged(const QRectF &)), SLOT( AdjustToSceneRect(const QRectF &) ));
                scene = ui->GetScene("Ether");
                if (scene)
                    connect(scene, SIGNAL( sceneRectChanged(const QRectF &)), SLOT( AdjustToSceneRect(const QRectF &) ));
                scene = ui->GetScene("Avatar");
                if (scene)
                    connect(scene, SIGNAL( sceneRectChanged(const QRectF &)), SLOT( AdjustToSceneRect(const QRectF &) ));
                hooked_to_scenes_ = true;
            }
            */
        }

        QGraphicsScene *current_scene = proxy_widget_->scene();
        if (!current_scene)
            return;

        visible_ = !visible_;
        int current_height = ui_view_->height()*0.5;
        if (visible_)
        {
            animation_.setStartValue(QRect(0, 0, ui_view_->width(), 0));
            animation_.setEndValue(QRect(0, 0, ui_view_->width(), current_height));
            // Not bringing to front, works in UiProxyWidgets, hmm...
            current_scene->setActiveWindow(proxy_widget_);
            current_scene->setFocusItem(proxy_widget_, Qt::ActiveWindowFocusReason);
            console_ui_->ConsoleInputArea->setFocus(Qt::MouseFocusReason);
            proxy_widget_->show();
        }
        else
        {
            animation_.setStartValue(QRect(0, 0, ui_view_->width(), current_height));
            animation_.setEndValue(QRect(0, 0, ui_view_->width(), 0));
            proxy_widget_->hide();
        }
        animation_.start();
    }

    void UiConsoleManager::StyleString(QString &str)
    {
        // Make all timestamp + module name blocks white
        int block_end_index = str.indexOf("]");
        if (block_end_index != -1)
        {
            QString span_start("<span style='color:white;'>");
            QString span_end("</span>");
            block_end_index += span_start.length() + 1;
            str.insert(0, "<span style='color:white;'>");
            str.insert(block_end_index, "</span>");
            block_end_index += span_end.length();
        }
        else
            block_end_index = 0;

        QRegExp regexp;
        regexp.setPattern(".*Debug:.*");
        if (regexp.exactMatch(str))
        {
            str.insert(block_end_index, "<FONT COLOR=\"#999999\">");
            str.push_back("</FONT>");
            return;
        }

        regexp.setPattern(".*Notice:.*");
        if (regexp.exactMatch(str))
        {
            str.insert(block_end_index, "<FONT COLOR=\"#0000FF\">");
            str.push_back("</FONT>");
            return;
        }

        regexp.setPattern(".*Warning:.*");
        if (regexp.exactMatch(str))
        {
            str.insert(block_end_index, "<FONT COLOR=\"#FFFF00\">");
            str.push_back("</FONT>");
            return;
        }

        regexp.setPattern(".*Error:.*");
        if (regexp.exactMatch(str))
        {
            str.insert(block_end_index, "<FONT COLOR=\"#FF3300\">");
            str.push_back("</FONT>");
            return;
        }

        regexp.setPattern(".*Critical:.*");
        if (regexp.exactMatch(str))
        {
            str.insert(block_end_index, "<FONT COLOR=\"#FF0000\">");
            str.push_back("</FONT>");
            return;
        }

        regexp.setPattern(".*Fatal:.*");
        if (regexp.exactMatch(str))
        {
            str.insert(block_end_index, "<FONT COLOR=\"#9933CC\">");
            str.push_back("</FONT>");
            return;
        }
    }
}
