//    Copyright (C) 2012 Dirk Vanden Boer <dirk.vdb@gmail.com>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#ifndef UPNP_ITEM_H
#define UPNP_ITEM_H

#include <string>
#include <vector>
#include <iostream>

#include "upnp/upnptypes.h"
#include "upnp/upnpprotocolinfo.h"
#include "upnp/upnpdlnainfo.h"

namespace upnp
{

class Resource
{
public:
    Resource();
    Resource(const Resource& other);
    Resource(Resource&& other);

    Resource& operator=(const Resource& other);
    Resource& operator=(Resource&& other);

    const std::string& getMetaData(const std::string& metaKey) const;
    const std::string& getUrl() const;
    const ProtocolInfo& getProtocolInfo() const;
    bool isThumbnail() const;

    void addMetaData(const std::string& key, const std::string& value);
    void setUrl(const std::string& url);
    void setProtocolInfo(const ProtocolInfo& info);

private:
    MetaMap         m_MetaData;
    std::string     m_Url;
    ProtocolInfo    m_ProtocolInfo;
};

inline std::ostream& operator<< (std::ostream& os, const Resource& res)
{
    return os << "Resource Url: " << res.getUrl() << std::endl
              << "ProtocolInfo: " << res.getProtocolInfo().toString() << std::endl;
}

class Item
{
public:
    enum Class
    {
        Container,
        VideoContainer,
        AudioContainer,
        ImageContainer,
        Video,
        Audio,
        Image,
        Generic,
        Unknown
    };

    explicit Item(const std::string& id = "0", const std::string& title = "");
    Item(const Item& other) = default;
    Item(Item&& other) = default;
    virtual ~Item();

    Item& operator= (const Item& other);
    Item& operator= (Item&& other);

    const std::string& getObjectId() const;
    const std::string& getParentId() const;
    const std::string& getRefId() const;
    std::string getTitle() const;
    bool restricted() const;
    
    // get the albumarturi with the specific profile id, returns an empty string if the profile is not present
    std::string getAlbumArtUri(dlna::ProfileId profile) const;

    const std::vector<Resource>& getResources() const;
    const std::map<dlna::ProfileId, std::string>& getAlbumArtUris() const;

    uint32_t getChildCount() const;
    Class getClass() const;
    std::string getClassString() const;
    void setClass(Class c);
    void setClass(const std::string& className);

    void setObjectId(const std::string& id);
    void setParentId(const std::string& id);
    void setRefId(const std::string& id);
    void setTitle(const std::string& title);
    void setChildCount(uint32_t count);

    void setAlbumArt(dlna::ProfileId profile, const std::string& uri);

    void addMetaData(Property prop, const std::string& value);
    void addResource(const Resource& resource);

    std::string getMetaData(Property prop) const;
    std::map<Property, std::string> getMetaData() const;

    friend std::ostream& operator<< (std::ostream& os, const Item& matrix);

private:
    std::string                             m_ObjectId;
    std::string                             m_ParentId;
    std::string                             m_RefId;

    bool                                    m_Restricted;
    
    std::map<Property, std::string>         m_MetaData;
    std::map<dlna::ProfileId, std::string>  m_AlbumArtUris;

    std::vector<Resource>                   m_Resources;
    uint32_t                                m_ChildCount;
};

inline std::ostream& operator<< (std::ostream& os, const Item& item)
{
    os << "Item: " << item.getTitle() << "(" << item.getObjectId() << ")" << std::endl
       << "Childcount: " << item.getChildCount() << std::endl
       << "Class: " << item.getClassString();

    for (auto& res : item.getResources())
    {
        os << res << std::endl;
    }

    if (item.getResources().empty()) os << std::endl;

    os << "Metadata:" << std::endl;
    for (auto& meta : item.m_MetaData)
    {
        os << toString(meta.first) << " - " << meta.second << std::endl;
    }

    return os;
}

}

#endif

