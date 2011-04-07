/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   VoiceToolWidget.h
 *  @brief  Widget for voice communication control
 *          
 */

#ifndef incl_UiModule_VoiceToolWidget_h
#define incl_UiModule_VoiceToolWidget_h

#include <QObject>
#include <QWidget>
#include <QLabel>
#include "ui_VoiceToolWidget.h"
#include "IToolWidget.h"
#include <QPushButton>
#include "InputAPI.h"
#include "VoiceController.h"

class UiProxyWidget;
class QComboBox;

namespace Foundation
{
    class Framework;
}

namespace Communications
{
    namespace InWorldVoice
    {
        class SessionInterface;
        class ParticipantInterface;
    }
}

namespace CommUI
{
    class VoiceStateWidget;
    class VoiceControllerWidget;
    class VoiceUsersInfoWidget;
    class VoiceTransmissionModeWidget;
    class VoiceController;

    class VoiceToolWidget : public CoreUi::IToolWidget , private Ui::voiceToolWidget
    {
        Q_OBJECT
    public:
        VoiceToolWidget(Foundation::Framework* framework);
        ~VoiceToolWidget();
    public slots:

        /// IToolWidget interface
        virtual void Minimize();

        /// IToolWidget interface
        virtual void Maximize();
    private slots:
        void ToggleVoiceControlWidget();
        void ToggleTransmissionModeWidget();
        void InitializeInWorldVoice();
        void ConnectInWorldVoiceSession(Communications::InWorldVoice::SessionInterface* session);
        void UpdateInWorldVoiceIndicator();
        void ConnectParticipantVoiceAvticitySignals(Communications::InWorldVoice::ParticipantInterface* p);
        void UninitializeInWorldVoice();
        void UpdateUI();
        void ChangeTransmissionMode(int mode);
        void UpdateTransmissionModeWidgetPosition();

    private:
        Foundation::Framework* framework_;
        Communications::InWorldVoice::SessionInterface* in_world_voice_session_;
        VoiceStateWidget* voice_state_widget_;
        VoiceUsersInfoWidget* voice_users_info_widget_;
        VoiceControllerWidget* voice_controller_widget_;
        UiProxyWidget* voice_controller_proxy_widget_;
        InputContextPtr input_context_;
        QComboBox* channel_selection_;
        VoiceTransmissionModeWidget* transmission_mode_widget_;

        VoiceController* voice_controller_;
    };

} // namespace incl_UiModule_VoiceToolWidget_h

#endif // incl_UiModule_VoiceToolWidget_h
