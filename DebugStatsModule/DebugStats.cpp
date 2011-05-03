/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   DebugStats.cpp
 *  @brief  DebugStatsModule shows information about internal core data structures in separate windows.
 *          Useful for verifying and understanding the internal state of the application.
 */

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "DebugStats.h"
#include "TimeProfilerWindow.h"
#include "ParticipantWindow.h"

#include "Framework.h"
#include "UiAPI.h"
#include "EventManager.h"
#include "ModuleManager.h"
#include "ConsoleCommandUtils.h"
#include "WorldStream.h"
#include "SceneAPI.h"
#include "SceneManager.h"
#include "Entity.h"
#include "NetworkEvents.h"
#include "RealXtend/RexProtocolMsgIDs.h"
#include "NetworkMessages/NetInMessage.h"
#include "NetworkMessages/NetMessageManager.h"
#include "Renderer.h"
#include "UiProxyWidget.h"
#include "EC_OpenSimPresence.h"
#include "ConsoleAPI.h"
#include "InputAPI.h"
#include "UiAPI.h"
#include "UiMainWindow.h"

#include <utility>
#include <QDebug>

#ifdef Q_WS_WIN
#include "Performance.h"
#endif 


#include <QCryptographicHash>

#include "MemoryLeakCheck.h"

using namespace std;

namespace DebugStats
{

const std::string DebugStatsModule::moduleName = std::string("DebugStats");

DebugStatsModule::DebugStatsModule() :
    IModule(NameStatic()),
    frameworkEventCategory_(0),
    networkEventCategory_(0),
    networkOutEventCategory_(0),
    networkStateEventCategory_(0),
    profilerWindow_(0),
    participantWindow_(0),
    godMode_(false)
{
}

DebugStatsModule::~DebugStatsModule()
{
    SAFE_DELETE(profilerWindow_);
}

void DebugStatsModule::PostInitialize()
{
#ifdef _WINDOWS
    QueryPerformanceCounter(&lastCallTime);
#endif

#ifdef PROFILING
    framework_->Console()->RegisterCommand("prof", "Shows the profiling window.", this, SLOT(ShowProfilingWindow()));

    framework_->Console()->RegisterCommand(CreateConsoleCommand("rin", 
        "Sends a random network message in.",
        ConsoleBind(this, &DebugStatsModule::SendRandomNetworkInPacket)));

    framework_->Console()->RegisterCommand(CreateConsoleCommand("rout", 
        "Sends a random network message out.",
        ConsoleBind(this, &DebugStatsModule::SendRandomNetworkOutPacket)));
#endif

    framework_->Console()->RegisterCommand(CreateConsoleCommand("Participant", 
        "Shows the participant window.",
        ConsoleBind(this, &DebugStatsModule::ShowParticipantWindow)));

    framework_->Console()->RegisterCommand(CreateConsoleCommand("iddqd",
        "Requests god-mode on from the server.",
        ConsoleBind(this, &DebugStatsModule::RequestGodMode)));

    framework_->Console()->RegisterCommand(CreateConsoleCommand("kick",
        "Kicks user out from the server. Usage: \"kick(fullname)\"",
        ConsoleBind(this, &DebugStatsModule::KickUser)));

    framework_->Console()->RegisterCommand(CreateConsoleCommand("dumptextures",
        "Dumps all currently existing J2K decoded textures as PNG files into the viewer working directory.",
        ConsoleBind(this, &DebugStatsModule::DumpTextures)));
        
    framework_->Console()->RegisterCommand(CreateConsoleCommand("exec",
        "Invokes action execution in entity",
        ConsoleBind(this, &DebugStatsModule::Exec)));

    frameworkEventCategory_ = framework_->GetEventManager()->QueryEventCategory("Framework");

    inputContext = framework_->Input()->RegisterInputContext("DebugStatsInput", 90);
    connect(inputContext.get(), SIGNAL(KeyPressed(KeyEvent *)), this, SLOT(HandleKeyPressed(KeyEvent *)));


//#ifdef Q_WS_WIN
// 
//    PDH::PerformanceMonitor monitor;
//    int treads = monitor.GetThreadCount();
//#endif 

    AddProfilerWidgetToUi();
}

void DebugStatsModule::HandleKeyPressed(KeyEvent *e)
{
    if (e->eventType != KeyEvent::KeyPressed || e->keyPressCount > 1)
        return;

    const QKeySequence showProfiler = framework_->Input()->KeyBinding("ShowProfilerWindow", QKeySequence(Qt::ShiftModifier + Qt::Key_P));
    if (QKeySequence(e->keyCode | e->modifiers) == showProfiler)
        ShowProfilingWindow();
}

void DebugStatsModule::AddProfilerWidgetToUi()
{
    if (profilerWindow_)
    {
        profilerWindow_->setVisible(!(profilerWindow_->isVisible()));
        return;
    }

    profilerWindow_ = new TimeProfilerWindow(framework_);
    profilerWindow_->setParent(framework_->Ui()->MainWindow());
    profilerWindow_->setWindowFlags(Qt::Tool);
    //profilerWindow_->move(100, 100);
    profilerWindow_->resize(650, 530);
    //UiProxyWidget *proxy = ui->AddWidgetToScene(profilerWindow_);
    connect(profilerWindow_, SIGNAL(Visible(bool)), SLOT(StartProfiling(bool)));

    //ui->AddWidgetToMenu(profilerWindow_, tr("Profiler"), tr("Developer Tools"), "./data/ui/images/menus/edbutton_MATWIZ_hover.png");
}

void DebugStatsModule::StartProfiling(bool visible)
{
    profilerWindow_->SetVisibility(visible);
    // -1 means start updating currently selected tab
    if (visible)
        profilerWindow_->OnProfilerWindowTabChanged(-1); 
}

ConsoleCommandResult DebugStatsModule::ShowProfilingWindow()
{
    // If the window is already created, bring it to front.
    if (profilerWindow_)
    {
        framework_->Ui()->BringWidgetToFront(profilerWindow_);
        return ConsoleResultSuccess();
    }
    else
        return ConsoleResultFailure("Profiler window has not been initialized, something went wrong on startup!");
}

ConsoleCommandResult DebugStatsModule::ShowParticipantWindow(const StringVector &params)
{
    if (participantWindow_)
    {
        framework_->Ui()->BringWidgetToFront(participantWindow_);
        return ConsoleResultSuccess();
    }

    participantWindow_ = new ParticipantWindow(framework_);
    participantWindow_->move(100, 100);
    participantWindow_->setWindowFlags(Qt::Dialog);

    framework_->Ui()->AddWidgetToScene(participantWindow_);
    framework_->Ui()->BringWidgetToFront(participantWindow_);

    return ConsoleResultSuccess();
}

void DebugStatsModule::Update(f64 frametime)
{
    RESETPROFILER;

#ifdef _WINDOWS
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double timeSpent = Foundation::ProfilerBlock::ElapsedTimeSeconds(lastCallTime.QuadPart, now.QuadPart);
    lastCallTime = now;

    frameTimes.push_back(make_pair(*(boost::uint64_t*)&now, timeSpent));
    if (frameTimes.size() > 2048) // Maintain an upper bound in the frame history.
        frameTimes.erase(frameTimes.begin());

    if (profilerWindow_)
    {
        if (!profilerWindow_->isVisible())
            return;
        profilerWindow_->RedrawFrameTimeHistoryGraph(frameTimes);
        profilerWindow_->DoThresholdLogging();
    }

#endif
}

bool DebugStatsModule::HandleEvent(event_category_id_t category_id, event_id_t event_id, IEventData *data)
{
    using namespace ProtocolUtilities;
    PROFILE(DebugStatsModule_HandleEvent);

    if (category_id == frameworkEventCategory_)
    {
        if (event_id == Foundation::WORLD_STREAM_READY)
        {
            WorldStreamReadyEvent *event_data = checked_static_cast<WorldStreamReadyEvent *>(data);
            assert(event_data);
            if (event_data)
                current_world_stream_ = event_data->WorldStream;
            if (profilerWindow_)
                profilerWindow_->SetWorldStreamPtr(current_world_stream_);

            networkEventCategory_ = framework_->GetEventManager()->QueryEventCategory("NetworkIn");
            networkOutEventCategory_ = framework_->GetEventManager()->QueryEventCategory("NetworkOut");
            networkStateEventCategory_ = framework_->GetEventManager()->QueryEventCategory("NetworkState");

            return false;
        }
    }

    if (category_id == networkStateEventCategory_)
    {
        switch(event_id)
        {
        case Events::EVENT_USER_CONNECTED:
        {
            UserConnectivityEvent *event_data = checked_static_cast<UserConnectivityEvent *>(data);
            assert(event_data);
            if (!event_data)
                return false;

            Scene::EntityPtr entity = GetFramework()->Scene()->GetDefaultScene()->GetEntity(event_data->localId);
            if (!entity)
                return false;

            EC_OpenSimPresence *ec_presence = entity->GetComponent<EC_OpenSimPresence>().get();
            if (ec_presence && participantWindow_)
                participantWindow_->AddUserEntry(ec_presence);
            break;
        }
        case Events::EVENT_USER_DISCONNECTED:
        {
            ProtocolUtilities::UserConnectivityEvent *event_data = checked_static_cast<UserConnectivityEvent *>(data);
            assert(event_data);
            if (!event_data)
                return false;

            Scene::EntityPtr entity = GetFramework()->Scene()->GetDefaultScene()->GetEntity(event_data->localId);
            if (!entity)
                return false;

            EC_OpenSimPresence *ec_presence = entity->GetComponent<EC_OpenSimPresence>().get();
            if (ec_presence && participantWindow_)
                participantWindow_->RemoveUserEntry(ec_presence);
            break;
        }
        default:
            break;
        }

        return false;
    }

    if (category_id == networkEventCategory_)
    {
        NetworkEventInboundData *netdata = checked_static_cast<NetworkEventInboundData *>(data);
        assert(netdata);
        if (!netdata)
            return false;

        if (event_id == RexNetMsgSimStats)
        {
            if (profilerWindow_)
                profilerWindow_->RefreshSimStatsData(netdata->message);
        }
        else if (event_id == RexNetMsgGrantGodlikePowers)
        {
            NetInMessage &msg = *netdata->message;
            RexUUID agent_id = msg.ReadUUID();
            RexUUID sessiont_id = msg.ReadUUID();
            uint8_t god_level = msg.ReadU8();
            if (god_level >= 200)
                LogInfo("God powers granted, level: " + QString::number(god_level).toStdString());
            else
                LogInfo("God powers denied");
        }

        if (profilerWindow_)
            profilerWindow_->LogNetInMessage(netdata->message);
    }
    if (category_id == networkOutEventCategory_)
    {
        NetworkEventOutboundData *netdata = checked_static_cast<NetworkEventOutboundData *>(data);
        assert(netdata);
        if (!netdata)
            return false;

        if (profilerWindow_)
            profilerWindow_->LogNetOutMessage(netdata->message);

        return false;
    }

    return false;
}

ConsoleCommandResult DebugStatsModule::SendRandomNetworkInPacket(const StringVector &params)
{
    if (params.size() == 0)
        return ConsoleResultSuccess();

    int numMessages = atoi(params[0].c_str());
    for(int i = 0; i < numMessages; ++i)
    {
        std::vector<char> data;
        int dataLen = rand() % 1600 + 1;
        for(int i = 0; i < dataLen; ++i)
            data.push_back(rand() % 256);

        int msgID = rand();

        ProtocolUtilities::NetMessageManager *messageManager = current_world_stream_->GetCurrentProtocolModule()->GetNetworkMessageManager();
        if (!messageManager)
            return ConsoleResultSuccess();

        try
        {
            ProtocolUtilities::NetInMessage msg(msgID, (boost::uint8_t *)&data[0], dataLen, false);
#ifdef _DEBUG
            msg.SetMessageID(msgID);
#endif
            ProtocolUtilities::NetMsgID id = msgID;

            ProtocolUtilities::NetworkEventInboundData inData(id , &msg);
            const ProtocolUtilities::NetMessageInfo *messageInfo = messageManager->GetMessageInfoByID(msgID);
            if (!messageInfo)
                continue;

            msg.SetMessageInfo(messageInfo);
            framework_->GetEventManager()->SendEvent(networkEventCategory_, id, &inData);
        }
        catch(const Exception &e)
        {
            LogInfo(std::string("Exception thrown: ") + e.what());
        }
        catch(const std::exception &e)
        {
            LogInfo(std::string("std::exception thrown: ") + e.what());
        }
        catch(...)
        {
            LogInfo("Unknown exception thrown.");
        }
    }
    return ConsoleResultSuccess();
}

ConsoleCommandResult DebugStatsModule::SendRandomNetworkOutPacket(const StringVector &params)
{
    return ConsoleResultSuccess();
}

ConsoleCommandResult DebugStatsModule::RequestGodMode(const StringVector &params)
{
    if (!current_world_stream_)
        return ConsoleResultFailure("Not connected to server.");

    current_world_stream_->SendRequestGodlikePowersPacket(true);
    return ConsoleResultSuccess();
}

ConsoleCommandResult DebugStatsModule::KickUser(const StringVector &params)
{
    if (!current_world_stream_)
        return ConsoleResultFailure("Not connected to server.");

    if (params.empty())
        return ConsoleResultFailure("Not enough parameters. Usage: \"kick(fullname)\"");

    Scene::ScenePtr scene = GetFramework()->Scene()->GetDefaultScene();
    if (!scene)
        return ConsoleResultFailure("No active scene found.");

    boost::shared_ptr<EC_OpenSimPresence> user_presence;
    Scene::EntityList users = scene->GetEntitiesWithComponent("EC_OpenSimPresence");
    foreach(Scene::EntityPtr entity, users)
    {
        boost::shared_ptr<EC_OpenSimPresence> ec_presence = entity->GetComponent<EC_OpenSimPresence>();
        assert(ec_presence);
        if (ec_presence->GetFullName().toStdString() == params[0])
        {
            user_presence = ec_presence;
            break;
        }
    }

    if (!user_presence)
        return ConsoleResultFailure("No user found with the given name.");;

    current_world_stream_->SendGodKickUserPacket(user_presence->agentId, "God doesn't want you here.");

    return ConsoleResultSuccess();
}

ConsoleCommandResult DebugStatsModule::DumpTextures(const StringVector &params)
{
    /* /// \todo Regression. Reimplement using the Asset API. -jj.

    boost::shared_ptr<OgreRenderer::Renderer> renderer = GetFramework()->GetServiceManager()->GetService
        <OgreRenderer::Renderer>(Service::ST_Renderer).lock();
    if (!renderer)
        return ConsoleResultFailure("No renderer");
    
    OgreRenderer::ResourceHandlerPtr res_handler = renderer->GetResourceHandler();
    std::vector<Foundation::ResourcePtr> textures = res_handler->GetResources(OgreRenderer::OgreTextureResource::GetTypeStatic());
    for (uint i = 0; i < textures.size(); ++i)
    {
        try
        {
            OgreRenderer::OgreTextureResource* tex_res = dynamic_cast<OgreRenderer::OgreTextureResource*>(textures[i].get());
            if (tex_res)
            {
                Ogre::Texture* ogre_tex = tex_res->GetTexture().get();
                if (ogre_tex)
                {
                    Ogre::Image new_image;
                    
                    // From Ogre 1.7 Texture::convertToImage()
                    size_t numMips = 1;
                    size_t dataSize = Ogre::Image::calculateSize(numMips,
                        ogre_tex->getNumFaces(), ogre_tex->getWidth(), ogre_tex->getHeight(), ogre_tex->getDepth(), ogre_tex->getFormat());
                    void* pixData = OGRE_MALLOC(dataSize, Ogre::MEMCATEGORY_GENERAL);
                    // if there are multiple faces and mipmaps we must pack them into the data
                    // faces, then mips
                    void* currentPixData = pixData;
                    for (size_t face = 0; face < ogre_tex->getNumFaces(); ++face)
                    {
                        for (size_t mip = 0; mip < numMips; ++mip)
                        {
                            size_t mipDataSize = Ogre::PixelUtil::getMemorySize(ogre_tex->getWidth(), ogre_tex->getHeight(), ogre_tex->getDepth(), ogre_tex->getFormat());
                            Ogre::PixelBox pixBox(ogre_tex->getWidth(), ogre_tex->getHeight(), ogre_tex->getDepth(), ogre_tex->getFormat(), currentPixData);
                            ogre_tex->getBuffer(face, mip)->blitToMemory(pixBox);
                            currentPixData = (void*)((char*)currentPixData + mipDataSize);
                        }
                    }
                    // load, and tell Image to delete the memory when it's done.
                    new_image.loadDynamicImage((Ogre::uchar*)pixData, ogre_tex->getWidth(), ogre_tex->getHeight(), ogre_tex->getDepth(), ogre_tex->getFormat(), true, 
                        ogre_tex->getNumFaces(), numMips - 1);
                    
                    // Hash the asset ID to generate filename, like assetcache does
                    const std::string& id = tex_res->GetId();
                    QCryptographicHash md5_engine_(QCryptographicHash::Md5);
                    md5_engine_.addData(id.c_str(), id.size());
                    QString md5_hash(md5_engine_.result().toHex());
                    md5_engine_.reset();
                    std::string hashed_name = md5_hash.toStdString();
                    
                    new_image.save(hashed_name + ".png");
                }
            }
        }
        catch (...)
        {
        }
    }
    */
    return ConsoleResultSuccess();
}

ConsoleCommandResult DebugStatsModule::Exec(const StringVector &params)
{
    if (params.size() < 2)
        return ConsoleResultFailure("Not enough parameters.");

    int id = ParseString<int>(params[0], 0);
    if (id == 0)
        return ConsoleResultFailure("Invalid value for entity ID. The ID must be an integer and unequal to zero.");

    Scene::ScenePtr scene = GetFramework()->Scene()->GetDefaultScene();
    if (!scene)
        return ConsoleResultFailure("No active scene.");

    Scene::EntityPtr entity = scene->GetEntity(id);
    if (!entity)
        return ConsoleResultFailure("No entity found for entity ID " + params[0]);

    QStringList execParameters;
    
    if (params.size() >= 3)
    {
        int type = ParseString<int>(params[2], 0);
        for(size_t i = 3; i < params.size(); ++i)
            execParameters << params[i].c_str();
        
        if (id != 0)
            entity->Exec((EntityAction::ExecutionType)type, params[1].c_str(), execParameters);
    }
    else
        entity->Exec(EntityAction::Local, params[1].c_str(), execParameters);

    return ConsoleResultSuccess();
}

}

extern "C" void POCO_LIBRARY_API SetProfiler(Foundation::Profiler *profiler);
void SetProfiler(Foundation::Profiler *profiler)
{
    Foundation::ProfilerSection::SetProfiler(profiler);
}

using namespace DebugStats;

POCO_BEGIN_MANIFEST(IModule)
    POCO_EXPORT_CLASS(DebugStatsModule)
POCO_END_MANIFEST 
