/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   ArgumentType.h
 *  @brief  
 */

#ifndef incl_ECEditorModule_ArgumentType_h
#define incl_ECEditorModule_ArgumentType_h

#include "ECEditorModuleApi.h"
#include "CoreStringUtils.h"

/// Pure virtual base class for different argument types.
class ECEDITOR_MODULE_API IArgumentType
{
public:
    /// Default constructor.
    IArgumentType() {}

    /// Destroys the object.
    virtual ~IArgumentType() {}

    /// Creates new editor for the argument type.
    /** @param parent Parent widget.
    */
    virtual QWidget *CreateEditor(QWidget *parent = 0) = 0;

    /// Reads value from the dedicated editor widget created by CreateEditor().
    virtual void UpdateValueFromEditor() = 0;

    /// Sets value to the editor, if editor exists.
    virtual void UpdateValueToEditor() = 0;

    /// Sets new value.
    /** @param val New value.
    */
    virtual void SetValue(void *val) = 0;

    /// Return arguments value as QGenericArgument.
    virtual QGenericArgument Value() = 0;

    /// Return arguments value as QGenericReturnArgument.
    virtual QGenericReturnArgument ReturnValue() = 0;

    /// Returns the arguments value a string.
    virtual QString ToString() const = 0;

    /// Returns the arguments value as a QVariant.
    virtual QVariant ToQVariant() const = 0;

    /// Sets value from QVariant.
    /** @param var New value as QVariant.
    */
    virtual void FromQVariant(const QVariant &var) = 0;

private:
    Q_DISABLE_COPY(IArgumentType);
};

/// Template implementation of IArgumentType.
template<typename T>
class ECEDITOR_MODULE_API ArgumentType : public IArgumentType
{
public:
    /// Constructor.
    /** @param name Type name.
    */
    ArgumentType(const char *name) : typeName(name), editor(0) {}

    /// IArgumentType override.
    virtual QWidget *CreateEditor(QWidget *parent = 0) { return 0; }

    /// IArgumentType override.
    virtual void UpdateValueFromEditor() {}

    /// IArgumentType override.
    virtual void UpdateValueToEditor() {}

    /// IArgumentType override.
    void SetValue(void *val) { value = *reinterpret_cast<T*>(val); }

    /// IArgumentType override.
    QGenericArgument Value() { return QGenericArgument(typeName.c_str(), static_cast<void*>(&value)); }

    /// IArgumentType override.
    QGenericReturnArgument ReturnValue() { return QGenericReturnArgument(typeName.c_str(), static_cast<void*>(&value)); }

    /// IArgumentType override.
    virtual QString ToString() const { return QString(); }

    /// IArgumentType override.
    virtual QVariant ToQVariant() const { return QVariant::fromValue<T>(value); }

    /// IArgumentType override.
    virtual void FromQVariant(const QVariant &var)
    {
        //assert(var.canConvert<T>());
        value = var.value<T>();
    }

private:
    std::string typeName; ///< Type name
    T value; ///< Value.
    QWidget *editor; ///< Editor widget dedicated for this argument type.
};

/// Void argument type. Used for return values only.
class ECEDITOR_MODULE_API VoidArgumentType : public IArgumentType
{
public:
    /// Default constructor.
    /** @param name Type name.
    */
    VoidArgumentType() : typeName("void") {}

    /// IArgumentType override. Returns 0.
    QWidget *CreateEditor(QWidget *parent = 0) { return 0; }

    /// IArgumentType override. Does nothing.
    void UpdateValueFromEditor() {}

    /// IArgumentType override. Does nothing.
    void UpdateValueToEditor() {}

    /// IArgumentType override. Does nothing.
    void SetValue(void *val) {}

    /// IArgumentType override. Returns empty QGenericArgument.
    QGenericArgument Value() { return QGenericArgument(); }

    /// IArgumentType override. Returns empty QGenericReturnArgument.
    QGenericReturnArgument ReturnValue() { return QGenericReturnArgument(); }

    /// IArgumentType override. Returns "void".
    QString ToString() const { return typeName; }

    /// IArgumentType override. Returns "void".
    QVariant ToQVariant() const { return QVariant(); }

    /// IArgumentType override. Does nothing.
    void FromQVariant(const QVariant &var) {}

private:
    QString typeName; ///< Type name
};

// QString
template<> QWidget *ArgumentType<QString>::CreateEditor(QWidget *parent);
template<> void ArgumentType<QString>::UpdateValueFromEditor();
template<> void ArgumentType<QString>::UpdateValueToEditor();
template<> QString ArgumentType<QString>::ToString() const;

// QStringList
template<> QWidget *ArgumentType<QStringList>::CreateEditor(QWidget *parent);
template<> void ArgumentType<QStringList>::UpdateValueFromEditor();
template<> void ArgumentType<QStringList>::UpdateValueToEditor();
template<> QString ArgumentType<QStringList>::ToString() const;

// std::string
template<> QWidget *ArgumentType<std::string>::CreateEditor(QWidget *parent);
template<> void ArgumentType<std::string>::UpdateValueFromEditor();
template<> void ArgumentType<std::string>::UpdateValueToEditor();
template<> QString ArgumentType<std::string>::ToString() const;
template<> QVariant ArgumentType<std::string>::ToQVariant() const;
template<> void ArgumentType<std::string>::FromQVariant(const QVariant &var);

// Boolean
template<> QWidget *ArgumentType<bool>::CreateEditor(QWidget *parent);
template<> void ArgumentType<bool>::UpdateValueFromEditor();
template<> void ArgumentType<bool>::UpdateValueToEditor();
template<> QString ArgumentType<bool>::ToString() const;

// Unsigned integer
template<> QWidget *ArgumentType<unsigned int>::CreateEditor(QWidget *parent);
template<> void ArgumentType<unsigned int>::UpdateValueFromEditor();
template<> void ArgumentType<unsigned int>::UpdateValueToEditor();
template<> QString ArgumentType<unsigned int>::ToString() const;

// Integer
template<> QWidget *ArgumentType<int>::CreateEditor(QWidget *parent);
template<> void ArgumentType<int>::UpdateValueFromEditor();
template<> void ArgumentType<int>::UpdateValueToEditor();
template<> QString ArgumentType<int>::ToString() const;

// Float
template<> QWidget *ArgumentType<float>::CreateEditor(QWidget *parent);
template<> void ArgumentType<float>::UpdateValueFromEditor();
template<> void ArgumentType<float>::UpdateValueToEditor();
template<> QString ArgumentType<float>::ToString() const;

// Double
template<> QWidget *ArgumentType<double>::CreateEditor(QWidget *parent);
template<> void ArgumentType<double>::UpdateValueFromEditor();
template<> void ArgumentType<double>::UpdateValueToEditor();
template<> QString ArgumentType<double>::ToString() const;

#endif
