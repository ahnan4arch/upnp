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

#pragma once

#include "upnp/upnpitem.h"
#include "upnp/upnpserviceclientbase.h"
#include "upnp/upnp.contentdirectory.types.h"

namespace upnp
{

class Device;
class IClient2;

typedef std::function<void(const Item&)> ItemCb;

namespace ContentDirectory
{

struct ServiceTraits
{
    using ActionType = ContentDirectory::Action;
    using VariableType = ContentDirectory::Variable;
    static const ServiceType SvcType = ServiceType::ContentDirectory;

    static ActionType actionFromString(const std::string& action);
    static const char* actionToString(ActionType action);
    static VariableType variableFromString(const std::string& var);
    static const char* variableToString(VariableType var);
};

class Client : public ServiceClientBase<ServiceTraits>
{
public:
    enum BrowseType
    {
        All,
        ItemsOnly,
        ContainersOnly
    };

    Client(upnp::IClient2& client);

    void setDevice(const std::shared_ptr<Device>& device) override;

    void abort();

    const std::vector<Property>& getSearchCapabilities() const;
    const std::vector<Property>& getSortCapabilities() const;

    void browseMetadata(const std::string& objectId, const std::string& filter, const std::function<void(int32_t, Item)> cb);
    void browseDirectChildren(BrowseType type, const std::string& objectId, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort, const std::function<void(int32_t, ActionResult)> cb);
    void search(const std::string& objectId, const std::string& criteria, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort, const std::function<void(int32_t, ActionResult)> cb);

protected:
    virtual std::chrono::seconds getSubscriptionTimeout() override;
    virtual void handleUPnPResult(int errorCode) override;

private:
    void browseAction(const std::string& objectId, const std::string& flag, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort, std::function<void(int32_t, std::string)> cb);

    void querySearchCapabilities();
    void querySortCapabilities();
    void querySystemUpdateID();

    std::vector<Property>       m_searchCaps;
    std::vector<Property>       m_sortCaps;
    std::string                 m_systemUpdateId;

    bool                        m_abort;
};

}
}
