#include "StableHeaders.h"
#include "DebugOperatorNew.h"
#include "MemoryLeakCheck.h"
#include "AvatarDescAsset.h"
#include "AssetAPI.h"

#include <QDomDocument>

AvatarDescAsset::~AvatarDescAsset()
{
    Unload();
}

void AvatarDescAsset::DoUnload()
{
    avatarAppearanceXML = "";
    assetReferences.clear();
}

bool AvatarDescAsset::DeserializeFromData(const u8 *data, size_t numBytes)
{
    // Just store the raw downloaded Avatar Appearance XML data for now..    
    QByteArray bytes((const char *)data, numBytes);
    avatarAppearanceXML = QString(bytes);
    // Parse the refs from the XML
    ParseReferences();

    return true;
}

bool AvatarDescAsset::SerializeTo(std::vector<u8> &dst, const QString &serializationParameters)
{
    ///\todo Implement.
    return false;
}

void AvatarDescAsset::ParseReferences()
{
    assetReferences.clear();
    
    QDomDocument avatar_doc("Avatar");
    if (!avatar_doc.setContent(avatarAppearanceXML))
        return;
    
    // The avatar desc itself is incomplete in regard to asset references. We need to parse the "assetmap"
    // which is an extension originally developed for inventory/webdav avatars
    
    QDomElement avatar = avatar_doc.firstChildElement("avatar");
    if (!avatar.isNull())
    {
        QDomElement assetmap = avatar.firstChildElement("assetmap");
        if (!assetmap.isNull())
        {
            QDomElement asset = assetmap.firstChildElement("asset");
            while (!asset.isNull())
            {
                AssetReference newRef;
                ///\todo The design of whether the LookupAssetRefToStorage should occur here, or internal to Asset API needs to be revisited.
                newRef.ref = assetAPI->LookupAssetRefToStorage(asset.attribute("id"));
                if (!newRef.ref.isEmpty())
                {
                    assetReferences.push_back(newRef);
                }
                
                asset = asset.nextSiblingElement("asset");
            }
        }
    }
}

bool AvatarDescAsset::IsLoaded() const
{
    return !avatarAppearanceXML.isEmpty();
}
