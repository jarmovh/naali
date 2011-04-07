// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_TundraLogicModule_Client_h
#define incl_TundraLogicModule_Client_h

#include "Core.h"
#include "TundraLogicModuleApi.h"
#include "ForwardDefines.h"

#include "kNet/Socket.h"
#include <QObject>

struct MsgLogin;
struct MsgLoginReply;
struct MsgClientJoined;
struct MsgClientLeft;

namespace kNet
{
class MessageConnection;
typedef unsigned long message_id_t;
}

namespace KristalliProtocol
{
    class KristalliProtocolModule;
}

class UserConnection;
typedef std::list<UserConnection*> UserConnectionList;

namespace TundraLogic
{

class TundraLogicModule;

class TUNDRALOGIC_MODULE_API Client : public QObject
{
    Q_OBJECT
    
    enum ClientLoginState
    {
        NotConnected = 0,
        ConnectionPending,
        ConnectionEstablished,
        LoggedIn
    };
    
public:
    //! Constructor
    Client(TundraLogicModule* owner, Foundation::Framework* fw);
    
    //! Destructor
    ~Client();
    
    //! Perform any per-frame processing
    void Update(f64 frametime);
    
    /// Get connection/login state
    ClientLoginState GetLoginState() { return loginstate_; }
    
    /// Returns the underlying kNet MessageConnection object that represents this connection.
    /// This function may return null in the case the connection is not active.
    kNet::MessageConnection* GetConnection();
    
    //! Handle Kristalli event
    void HandleKristalliEvent(event_id_t event_id, IEventData* data);
    
signals:
    /// This signal is emitted right before this client is starting to connect to a Tundra server.
    /// Any script or other piece of code can listen to this signal, and as at this point, fill in any internal
    /// custom data (called "login properties") they need to add to the connection handshake. The server will get 
    /// all the login properties and a server-side script can do validation and storage of whether the client
    /// can be authorized to log in or not.
    void AboutToConnect();

    /// This signal is emitted immediately after this client has successfully connected to a server.
    void Connected();
    
    /// This signal is emitted when the client has disconnected from the server.
    void Disconnected();
    
public slots:
    /// Connect and login. The QUrls query parameters will be evaluated for the login data.
    /// Minimum information needed to try a connection in the url are host and username.
    /// Url syntax: {tundra|http|https}://host[:port]/?username=x[&password=y&avatarurl=z&protocol={udp|tcp}]
    /// Url examples: tundra://server.com/?username=John tundra://server.com:5432/?username=John&password=pWd123&protocol=udp 
    /// \param QUrl the login url.
    /// \note The port where we perform the login will be gotten from urls port, not from a query parameter. If port is not present we default to tundras default 2345.
    void Login(const QUrl& loginUrl);

    /// Connect and login. Username and password will be encoded to xml key-value data
    /// \note This function will be deleted in the future.
    void Login(const QString& address, unsigned short port, const QString& username, const QString& password, const QString &protocol = QString());

    /// Connect and login using the login properties that were previously set with calls to SetLoginProperty.
    void Login(const QString& address, unsigned short port, kNet::SocketTransportLayer protocol = kNet::InvalidTransportLayer);
    
    /// Disconnects the client from the current server, and also deletes all contents from the client scene.
    /// \param fail Pass in true if the logout was due to connection/login failure. False, if the connection was aborted deliberately by the client.
    void Logout(bool fail = false);
    
    /// Get client connection ID (from loginreply message). Is zero if not connected
    int GetConnectionID() const { return client_id_; }
    
    //! See if connected & authenticated
    bool IsConnected() const;
    
    /// Sets the given login property with the given value. Call this function prior connecting to a scene to specify
    /// data that should be carried to the server as initial login data.
    /// @param key The name of the login property to set. If a previous login property with this name existed,
    ///          it is overwritten.
    /// @param value The value to specify for this login property. If "", the login property is deleted and will not be sent.
    void SetLoginProperty(QString key, QString value);

    /// Returns the login property value of the given name.
    QString GetLoginProperty(QString key);
    
    /// Returns all the currently set login properties as Xml text.
    QString LoginPropertiesAsXml();

    /// Returns all the login properties that will be used to login to the server.
    std::map<QString, QString> &GetAllLoginProperties() { return properties; }

    /// Deletes all set login properties.
    void ClearLoginProperties() { properties.clear(); }

private:
    /// Handle a Kristalli protocol message
    void HandleKristalliMessage(kNet::MessageConnection* source, kNet::message_id_t id, const char* data, size_t numBytes);
    
    /// Handle pending login to server
    void CheckLogin();
    
    /// Handle a loginreply message
    void HandleLoginReply(kNet::MessageConnection* source, const MsgLoginReply& msg);
    
    /// Handle a client joined message
    void HandleClientJoined(kNet::MessageConnection* source, const MsgClientJoined& msg);
    
    /// Client: Handle a client left message
    void HandleClientLeft(kNet::MessageConnection* source, const MsgClientLeft& msg);
    
    /// Client's connection/login state
    ClientLoginState loginstate_;
    /// Specifies all the login properties.
    std::map<QString, QString> properties;
    /// Whether the connect attempt is a reconnect because of dropped connection
    bool reconnect_;
    /// User ID, once known
    u8 client_id_;
    
    /// Kristalli event category
    event_category_id_t kristalliEventCategory_;
    /// Tundra event category
    event_category_id_t tundraEventCategory_;
    
    //! Owning module
    TundraLogicModule* owner_;
    //! Framework pointer
    Foundation::Framework* framework_;
};

}
#endif
