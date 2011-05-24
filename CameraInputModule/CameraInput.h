// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_CameraInputModule_CameraInput_h
#define incl_CameraInputModule_CameraInput_h

#include "CameraInputModuleApi.h"

#include <QObject>
#include <QImage>

class CameraInputModule;
namespace Foundation { class Framework; }

class CAMERAINPUT_MODULE_API CameraInput : public QObject
{

Q_OBJECT

public:
    friend class CameraInputModule;

    /// Default constructor.
    CameraInput(CameraInputModule *cameraInputModule, Foundation::Framework *framework);

    /// Destructor.
    virtual ~CameraInput();

public slots:
    /// Returns if we have a active device.
    /// \return bool True if we have a device and capturing can be started.
    bool HasDevice();

    /// Return if video is being captured.
    /// \return bool True if capturing false other wise.
    bool IsCapturing();

    /// Start capturing.
    void StartCapturing();

    /// Stop capturing.
    void StopCapturing();

    /// Get the capture frames per second.
    /// \return int Capture fps.
    int GetCaptureFps() const;

    /// Set the capture frames per second. Default value is 15 set by CameraInputModule on its creation.
    /// \note Be careful going above 20 if you are using the FrameUpdate signal in scripting languages.
    /// \note This function will enforce fps to be between 1 and 60.
    /// \param int New capture fps.
    void SetCaptureFps(int fps);

    /// Return the current captured frame.
    /// \return QImage The current captured frame.
    const QImage &CurrentFrame();

signals:
    /// This signal is emitted when the current captured frame is updated.
    /// \param QImage Updated frame, this is always the current frame of CameraInput.
    void FrameUpdate(const QImage &frame);

    /// This signal is emitted when capturing state changes.
    /// \param bool True if capturing false other wise.
    void Capturing(bool capturing);

protected:
    /// Protected function that gets called by the friend class CameraInputModule.
    void SetEnabled(bool enabled);
    
    /// Protected function that gets called by the friend class CameraInputModule.
    void SetFrame(const QImage &frame);

private:
    Foundation::Framework *framework_;
    CameraInputModule *cameraInputModule_;
    bool hasDevice_;
    bool capturing_;
    QImage currentFrame_;
};

#endif