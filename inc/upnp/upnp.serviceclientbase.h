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

#include "utils/log.h"
#include "utils/signal.h"

#include "upnp/asio.h"
#include "upnp/upnp.clientinterface.h"
#include "upnp/upnp.action.h"
#include "upnp/upnp.device.h"
#include "upnp/upnp.xml.parseutils.h"

#include <set>
#include <map>
#include <vector>
#include <memory>

namespace upnp
{

template <typename Traits>
class ServiceClientBase
{
public:
    utils::Signal<typename Traits::VariableType, const std::map<typename Traits::VariableType, std::string>&> StateVariableEvent;

    ServiceClientBase(IClient& client)
    : m_client(client)
    , m_subTimer(client.ioService())
    {
    }

    virtual ~ServiceClientBase() = default;

    virtual Future<void> setDevice(const std::shared_ptr<Device>& device)
    {
        if (device->implementsService(serviceType()))
        {
            m_service = device->services[Traits::SvcType];
            co_await processServiceDescription(m_service.scpdUrl);
        }
        else
        {
            throw std::invalid_argument("Device does not implement interface");
        }
    }

    virtual void setDevice(const std::shared_ptr<Device>& device, std::function<void(Status)> cb)
    {
        if (device->implementsService(serviceType()))
        {
            m_service = device->services[Traits::SvcType];
            processServiceDescription(m_service.scpdUrl, cb);
        }
        else
        {
            cb(Status(ErrorCode::InvalidArgument, "Device does not implement interface"));
        }
    }

    void subscribe(std::function<void(Status)> cb)
    {
        m_client.subscribeToService(m_service.eventSubscriptionURL, getSubscriptionTimeout(), [this, cb] (Status status, const std::string& subId, std::chrono::seconds subTimeout) -> std::function<void(SubscriptionEvent)> {
            if (!status)
            {
                utils::log::error("Error subscribing to service: {}", status.what());
                cb(status);
                return nullptr;
            }

            m_subscriptionId = subId;

            if (subTimeout.count() > 0) // 0 timeout is infinite subscription, no need to renew
            {
                m_subTimer.expires_from_now(subTimeout * 3 / 4);
                m_subTimer.async_wait([this, subTimeout] (const boost::system::error_code& e) {
                    if (e != asio::error::operation_aborted)
                    {
                        renewSubscription(subTimeout);
                    }
                });
            }

            cb(status);
            return std::bind(&ServiceClientBase<Traits>::eventCb, this, std::placeholders::_1);
        });
    }

    void unsubscribe(std::function<void(Status)> cb)
    {
        m_client.ioService().post([this] () {
            m_subTimer.cancel();
        });

        m_client.unsubscribeFromService(m_service.eventSubscriptionURL, m_subscriptionId, cb);
    }

    bool supportsAction(typename Traits::ActionType action) const
    {
        return m_supportedActions.find(action) != m_supportedActions.end();
    }

    static constexpr ServiceType serviceType()
    {
        return ServiceType { Traits::SvcType, Traits::SvcVersion };
    }

protected:
    virtual Future<void> processServiceDescription(const std::string& descriptionUrl)
    {
        auto contents = co_await m_client.getFile(descriptionUrl);
        m_stateVariables = xml::parseServiceDescription(contents, [this] (const std::string& action) {
            try
            {
                m_supportedActions.insert(actionFromString(action));
            }
            catch (const std::exception& e)
            {
                utils::log::warn(e.what());
            }
        });
    }

    virtual void processServiceDescription(const std::string& descriptionUrl, std::function<void(Status)> cb)
    {
        m_client.getFile(descriptionUrl, [this, cb] (Status status, const std::string& contents) {
            if (status)
            {
                try
                {
                    m_stateVariables = xml::parseServiceDescription(contents, [this] (const std::string& action) {
                        try
                        {
                            m_supportedActions.insert(actionFromString(action));
                        }
                        catch (const std::exception& e)
                        {
                            utils::log::warn(e.what());
                        }
                    });
                }
                catch (const std::exception& e)
                {
                    status = Status(ErrorCode::Unexpected, e.what());
                }
            }
            else
            {
                utils::log::error("Failed to download service description: {}", status.what());
            }

            cb(status);
        });
    }

    void executeAction(typename Traits::ActionType actionType, std::function<void(Status, std::string)> cb)
    {
        executeAction(actionType, std::map<std::string, std::string> {}, std::move(cb));
    }

    Future<std::string> executeAction(typename Traits::ActionType actionType, const std::map<std::string, std::string>& args)
    {
        Action action(actionToString(actionType), m_service.controlURL, serviceType());
        for (auto& arg : args)
        {
            action.addArgument(arg.first, arg.second);
        }

        auto res = co_await m_client.sendAction(action);

        // TODO: http status gets lost here
        if (res.fault)
        {
            throw Status(ErrorCode::SoapError, res.fault->errorCode(), res.fault->errorDescription());
        }
        else
        {
            co_return std::move(res.response.body);
        }
    }

    void executeAction(typename Traits::ActionType actionType, const std::map<std::string, std::string>& args, std::function<void(Status, std::string)> cb)
    {
        Action action(actionToString(actionType), m_service.controlURL, serviceType());
        for (auto& arg : args)
        {
            action.addArgument(arg.first, arg.second);
        }

        m_client.sendAction(action, [cb] (Status status, soap::ActionResult res) {
            assert(cb);

            // TODO: http status gets lost here
            if (res.fault)
            {
                cb(Status(ErrorCode::SoapError, res.fault->errorCode(), res.fault->errorDescription()), std::move(res.response.body));
            }
            else
            {
                cb(status, std::move(res.response.body));
            }
        });
    }

    virtual std::chrono::seconds getSubscriptionTimeout() = 0;
    virtual void handleStateVariableEvent(typename Traits::VariableType /*changedVariable*/, const std::map<typename Traits::VariableType, std::string>& /*variables*/) {}

    std::vector<StateVariable> m_stateVariables;

private:
    typename Traits::ActionType actionFromString(const std::string& action)
    {
        return Traits::actionFromString(action);
    }

    std::string actionToString(typename Traits::ActionType action)
    {
        return Traits::actionToString(action);
    }

    typename Traits::VariableType variableFromString(const std::string& var)
    {
        return Traits::variableFromString(var);
    }

    std::string variableToString(typename Traits::VariableType var)
    {
        return Traits::variableToString(var);
    }

    void renewSubscription(std::chrono::seconds timeout)
    {
        m_client.renewSubscription(m_service.eventSubscriptionURL, m_subscriptionId, timeout, [this] (int32_t status, std::string, auto timeout) {
            if (!status)
            {
                utils::log::error("Failed to renew subscription");
            }
            else
            {
                m_subTimer.expires_from_now(timeout * 3 / 4);
                m_subTimer.async_wait([this, timeout] (const boost::system::error_code& e) {
                    if (e !=  asio::error::operation_aborted && timeout.count() > 0)
                    {
                        this->renewSubscription(timeout);
                    }
                });
            }
        });
    }

    void eventCb(const SubscriptionEvent& event)
    {
        //utils::log::info(event.data);

        try
        {
            xml::parseEvent(event.data, [this] (const std::string& varType, const std::map<std::string, std::string>& values) {
                auto changedVar = variableFromString(varType);

                std::map<typename Traits::VariableType, std::string> vars;
                for (auto& val : values)
                {
                    vars.emplace(variableFromString(val.first), xml::decode(val.second));
                }

                // let the service implementation process the event if necessary
                handleStateVariableEvent(changedVar, vars);

                // notify clients
                StateVariableEvent(changedVar, vars);
            });
        }
        catch (std::exception& e)
        {
            utils::log::error("Failed to parse event: {}", e.what());
        }
    }

    IClient&                                m_client;
    Service                                 m_service;
    asio::steady_timer                      m_subTimer;
    std::set<typename Traits::ActionType>   m_supportedActions;
    std::string                             m_subscriptionId;
};

}
