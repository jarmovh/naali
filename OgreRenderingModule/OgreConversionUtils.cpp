/** 
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   OgreConversionUtils.cpp
 *  @brief  Contains some common methods for conversions between Ogre and Core variable types.
 */

#include "StableHeaders.h"
#include "DebugOperatorNew.h"
#include "CoreStringUtils.h"
#include "OgreConversionUtils.h"
#include "MemoryLeakCheck.h"

namespace OgreRenderer
{

Ogre::ColourValue ToOgreColor(const Color& color)
{
    return Ogre::ColourValue(color.r, color.g, color.b, color.a);
}

Color ToCoreColor(const Ogre::ColourValue& ogreColour)
{
   return Color(ogreColour.r, ogreColour.g, ogreColour.b, ogreColour.a);
}

Ogre::Matrix4 OGRE_MODULE_API ToOgreMatrix4(const Matrix4 &matrix)
{
    return Ogre::Matrix4(
        matrix[0], matrix[4], matrix[8], matrix[12],
        matrix[1], matrix[5], matrix[9], matrix[13],
        matrix[2], matrix[6], matrix[10], matrix[14],
        matrix[3], matrix[7], matrix[11], matrix[15]);
}

Ogre::Vector3 ToOgreVector3(const Vector3df &vector)
{
    return Ogre::Vector3(vector.x, vector.y, vector.z);
}

Ogre::Quaternion ToOgreQuaternion(const Quaternion &quat)
{
    return Ogre::Quaternion(quat.w, quat.x, quat.y, quat.z);
}

std::string SanitateAssetIdForOgre(const QString& input)
{
    return SanitateAssetIdForOgre(input.toStdString());
}

std::string SanitateAssetIdForOgre(const std::string& input)
{
    std::string ret = input;
    ReplaceCharInplace(ret, ':', '_');
    ReplaceCharInplace(ret, '/', '_');
    return ret;
}

std::string SanitateAssetIdForOgre(const char* input)
{
    if (!input)
        return std::string();
    return SanitateAssetIdForOgre(std::string(input));
}

}
