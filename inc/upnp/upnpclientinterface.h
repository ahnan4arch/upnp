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

#ifndef UPNP_CLIENT_INTERFACE_H
#define UPNP_CLIENT_INTERFACE_H

#include <string>

#include "utils/signal.h"

#include "upnp/upnptypes.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnpaction.h"
#include "upnp/upnpxmlutils.h"

struct Upnp_Event;

namespace upnp
{

struct DeviceDiscoverInfo
{
    uint32_t        expirationTime;
    std::string     deviceId;
    std::string     deviceType;
    std::string     serviceType;
    std::string     serviceVersion;
    std::string     location;
};

class IServiceSubscriber
{
public:
    virtual ~IServiceSubscriber() = default;
    virtual void onServiceEvent(Upnp_EventType eventType, void* pEvent) = 0;
    virtual std::string getSubscriptionId() = 0;
};

class IClient
{
public:
    virtual ~IClient() {}

    virtual void initialize(const char* interfaceName = nullptr, int32_t port = 0) = 0;
    virtual void destroy() = 0;
    virtual void reset() = 0;

    virtual std::string getIpAddress() const = 0;
    virtual int32_t getPort() const = 0;
    virtual void searchDevicesOfType(DeviceType type, int32_t timeout) const = 0;
    virtual void searchAllDevices(int32_t timeout) const = 0;

    // synchronously subscribe to the service, returns the subscription id
    virtual std::string subscribeToService(const std::string& publisherUrl, int32_t& timeout) const = 0;
    // synchronously unsubscribe from the service
    virtual void unsubscribeFromService(const std::string& subscriptionId) const = 0;

    // asynchronously subscribe to the service
    virtual void subscribeToService(const std::string& publisherUrl, int32_t timeout, const std::shared_ptr<IServiceSubscriber>& sub) const = 0;
    // synchronously unsubscribe from the service
    virtual void unsubscribeFromService(const std::shared_ptr<IServiceSubscriber>& sub) const = 0;

    virtual xml::Document sendAction(const Action& action) const = 0;
    virtual xml::Document downloadXmlDocument(const std::string& url) const = 0;

    utils::Signal<const DeviceDiscoverInfo&> UPnPDeviceDiscoveredEvent;
    utils::Signal<const std::string&> UPnPDeviceDissapearedEvent;
    utils::Signal<Upnp_Event*> UPnPEventOccurredEvent;
};

}

#endif

