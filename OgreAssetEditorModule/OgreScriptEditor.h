/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   OgreScriptEditor.h
 *  @brief  Editing tool for OGRE material and particle scripts.
 *          Provides raw text edit for particles and QProperty editing for materials.
 */

#ifndef incl_OgreAssetEditorModule_OgreScriptEditor_h
#define incl_OgreAssetEditorModule_OgreScriptEditor_h

#include "RexTypes.h"
#include "OgreAssetEditorModuleApi.h"
#include "AssetFwd.h"

#include <boost/shared_ptr.hpp>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QPushButton;
class QLineEdit;
class QTextEdit;
QT_END_NAMESPACE

namespace Foundation
{
    class Framework;
}

namespace Inventory
{
    class InventoryUploadBufferEventData;
}

class OgreMaterialProperties;
class PropertyTableWidget;

class ASSET_EDITOR_MODULE_API OgreScriptEditor : public QWidget
{
    Q_OBJECT

public:
    /** Constructor.
        @param inventory_id Inventory ID unique for this item.
        @param asset_type Asset type.
        @param name Name.
        @param parent Parent widget.
    */
    OgreScriptEditor(
        const QString &inventory_id,
        const asset_type_t &asset_type,
        const QString &name,
        QWidget *parent = 0);

    /// Destructor.
    virtual ~OgreScriptEditor();

    static void OpenOgreScriptEditor(Foundation::Framework *framework, const QString &asset_id, asset_type_t asset_type, QWidget* parent = 0);

public slots:
    void Open();

    /// Handles the asset data for script.
//    void HandleAssetReady(Foundation::AssetInterfacePtr asset);

    /// Closes the window.
    void Close();

signals:
    /// This signal is emitted when the editor is closed.
    void Closed(const QString &inventory_id, asset_type_t asset_type);

    /// This signal is emitted user saves modifications to a script and uploads it as a new one.
    void UploadNewScript(Inventory::InventoryUploadBufferEventData *data);

private slots:
    /// Save As
    void SaveAs();

    /// Validates the script name
    /// @param name Name.
    void ValidateScriptName(const QString &name);

    /// Validates the propertys new value.
    /// @param row Row of the cell.
    /// @param column Column of the cell.
    void PropertyChanged(int row, int column);

    //! Delete this object.
    void Deleted() { delete this; }

private:
    Q_DISABLE_COPY(OgreScriptEditor);

    /// Creates the text edit field for raw editing.
    void CreateTextEdit();

    /// Creates the property table for material property editing.
    void CreatePropertyEditor();

    /// Main widget loaded from .ui file.
    QWidget *mainWidget_;

    /// Save As button.
    QLineEdit *lineEditName_;

    /// Save As button.
    QPushButton *buttonSaveAs_;

    /// Cancel button.
    QPushButton *buttonCancel_;

    /// Text edit field used in raw edit mode.
    QTextEdit *textEdit_;

    /// Table widget for editing material properties.
    PropertyTableWidget *propertyTable_;

    /// Inventory id.
    QString inventoryId_;

    /// Asset type.
    const asset_type_t assetType_;

    /// Script name.
    QString name_;

    /// Material properties.
    OgreMaterialProperties *materialProperties_;
};

#endif
