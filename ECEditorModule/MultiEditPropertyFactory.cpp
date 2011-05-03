#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "MultiEditPropertyFactory.h"
#include "qteditorfactory.h"
#include <QLayout>

#include "Application.h"

#include "MemoryLeakCheck.h"

MultiEditPropertyFact::MultiEditPropertyFact(QObject *parent):
    QtAbstractEditorFactory<MultiEditPropertyManager>(parent)
{
    
}

MultiEditPropertyFact::~MultiEditPropertyFact()
{
    qDeleteAll(editorToProperty_.keys());
}

void MultiEditPropertyFact::connectPropertyManager(MultiEditPropertyManager *manager)
{
    /*QObject::connect(manager, SIGNAL(AttributeValuesUpdated(const QtProperty *, const QStringList &)),
                        this, SLOT(UpdateAttributeValues(const QtProperty *, const QStringList &)));*/
}

QWidget *MultiEditPropertyFact::createEditor(MultiEditPropertyManager *manager, QtProperty *property, QWidget *parent)
{
    QPushButton *multiEditButton = new QPushButton(parent);

    /// \bug QInputContext has a bug where if you have a QInputDialog in a QGraphicsView and close the dialog, the QInputContext
    /// will access a dangling pointer and crash. This bug can be avoided by never freeing the QInputDialog (and leaking),
    /// which allows QInputContext::setFocusWidget to go through. The following commented out line would be preferred, but
    /// at the moment is not possible. See http://clb.demon.fi/dump/QWinInputContextUpdateCrash.png . -jj.
//        QInputDialog *dialog = new QInputDialog(parent);
    // Now as a workaround, create the input dialog without a parent so that we won't crash.
    QInputDialog *dialog = new QInputDialog();

    QStringList attributes = manager->AttributeValue(property);
    dialog->setComboBoxItems(attributes);
    dialog->setInputMode(QInputDialog::TextInput);
    dialog->setComboBoxEditable(true);
    QObject::connect(multiEditButton, SIGNAL(clicked()), dialog, SLOT(open()));
    multiEditButton->setText(QString("(%1 values)").arg(attributes.size()));

    createdEditors_[property] = dialog;
    editorToProperty_[dialog] = property;
    QObject::connect(dialog, SIGNAL(textValueSelected(const QString &)), 
                     this, SLOT(DialogValueSelected(const QString &)));
    QObject::connect(this, SIGNAL(ValueSelected(QtProperty *, const QString &)), 
                     manager, SLOT(SetValue(QtProperty *, const QString &)));
    QObject::connect(dialog, SIGNAL(destroyed(QObject *)),
                     this, SLOT(EditorDestroyed(QObject *)));

    return multiEditButton;
}

void MultiEditPropertyFact::disconnectPropertyManager(MultiEditPropertyManager *manager)
{
    /*QObject::disconnect(manager, SIGNAL(AttributeValuesUpdated(const QtProperty *, const QStringList &)),
                           this, SLOT(UpdateAttributeValues(const QtProperty *, const QStringList &)));*/
}

void MultiEditPropertyFact::DialogValueSelected(const QString &value)
{
    QInputDialog *dialog = dynamic_cast<QInputDialog *>(QObject::sender());
    if(!dialog)
        return;
    if(!editorToProperty_.contains(dialog))
        return;
    QtProperty *property = const_cast<QtProperty *>(editorToProperty_[dialog]);

    emit ValueSelected(property, value);
}

void MultiEditPropertyFact::EditorDestroyed(QObject *object)
{
    QMap<QDialog *, const QtProperty *>::ConstIterator iter = editorToProperty_.constBegin();
    while (iter != editorToProperty_.constEnd()) 
    {
        if (iter.key() == object)
        {
            QDialog *dialog = iter.key();
            const QtProperty *property = iter.value();
            editorToProperty_.remove(dialog);
            createdEditors_.remove(property);
            break;
        }
        iter++;
    }
}