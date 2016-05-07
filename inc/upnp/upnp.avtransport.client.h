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

#include "utils/signal.h"
#include "upnp/upnpserviceclientbase.h"
#include "upnp/upnp.avtransport.types.h"

namespace upnp
{

class Item;
class Action;

namespace AVTransport
{

struct ServiceTraits
{
    using ActionType = AVTransport::Action;
    using VariableType = AVTransport::Variable;
    static const ServiceType SvcType = ServiceType::AVTransport;

    static Action actionFromString(const std::string& action);
    static const char* actionToString(Action action);
    static Variable variableFromString(const std::string& var);
    static const char* variableToString(Variable var);
};

class Client : public ServiceClientBase<ServiceTraits>
{
public:
    Client(IClient2& client);

    void setAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData, std::function<void(int32_t)> cb);
    void setNextAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData, std::function<void(int32_t)> cb);

    void play(int32_t connectionId, const std::string& speed, std::function<void(int32_t)> cb);
    void pause(int32_t connectionId, std::function<void(int32_t)> cb);
    void stop(int32_t connectionId, std::function<void(int32_t)> cb);
    void previous(int32_t connectionId, std::function<void(int32_t)> cb);
    void seek(int32_t connectionId, SeekMode mode, const std::string& target, std::function<void(int32_t)> cb);
    void next(int32_t connectionId, std::function<void(int32_t)> cb);

    void getPositionInfo(int32_t connectionId, std::function<void(int32_t, PositionInfo)> cb);
    void getMediaInfo(int32_t connectionId, std::function<void(int32_t, MediaInfo)> cb);
    void getTransportInfo(int32_t connectionId, std::function<void(int32_t, TransportInfo)> cb);
    void getCurrentTransportActions(int32_t connectionId, std::function<void(int32_t, std::set<Action>)> cb);

    utils::Signal<const std::map<Variable, std::string>&> LastChangeEvent;

protected:
    void handleStateVariableEvent(Variable var, const std::map<Variable, std::string>& variables) override;

    std::chrono::seconds getSubscriptionTimeout() override;
};

}
}