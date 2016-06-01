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

#include "upnp.client.h"

#include "utils/log.h"
#include "upnp/upnp.uv.h"
#include "upnp/upnp.action.h"
#include "upnp.gena.server.h"

#include <stdexcept>
#include <algorithm>
#include <cstring>

using namespace utils;
using namespace std::placeholders;

//#define DEBUG_UPNP_CLIENT

namespace upnp
{

Status httpStatusToStatus(int32_t httpStatus)
{
    if (httpStatus < 0)
    {
        return Status(ErrorCode::NetworkError, uv::getErrorString(httpStatus));
    }
    else if (httpStatus != 200)
    {
        return Status(ErrorCode::HttpError, http::Client::errorToString(httpStatus));
    }

    return Status();
}

Client2::Client2()
: m_loop(std::make_unique<uv::Loop>())
{
}

Client2::~Client2() = default;

void Client2::initialize()
{
    log::debug("Initializing UPnP SDK");
    return initialize(uv::Address::createIp4("0.0.0.0", 0));
}

void Client2::initialize(const std::string& interfaceName, uint16_t port)
{
    log::debug("Initializing UPnP SDK");
    auto addr = uv::Address::createIp4(interfaceName);
    addr.setPort(port);
    return initialize(addr);
}

void Client2::initialize(const uv::Address& addr)
{
    m_httpClient = std::make_unique<http::Client>(*m_loop);
    m_eventServer = std::make_unique<gena::Server>(*m_loop, addr, [&] (const SubscriptionEvent& ev) {
        auto iter = m_eventCallbacks.find(ev.sid);
        if (iter != m_eventCallbacks.end())
        {
            iter->second(ev);
        }
    });

    m_thread = std::make_unique<std::thread>([this] () {
        auto curlInit = curl_global_init(CURL_GLOBAL_WIN32);
        if (curlInit)
        {
            throw std::runtime_error("Failed to init curl library");
        }

        m_loop->run(upnp::uv::RunMode::Default);
        curl_global_cleanup();
    });
}

void Client2::uninitialize()
{
    log::debug("Uninitializing UPnP SDK");
    uv::asyncSend(*m_loop, [&] () {
        m_httpClient.reset();
        m_eventServer->stop([this] () {
            m_eventServer.reset();
            stopLoopAndCloseRequests(*m_loop);
        });
    });

    m_thread->join();

    m_thread.reset();
}

std::string Client2::getIpAddress() const
{
    return m_eventServer->getAddress().ip();
}

uint16_t Client2::getPort() const
{
    return m_eventServer->getAddress().port();
}

void Client2::subscribeToService(const std::string& publisherUrl, std::chrono::seconds timeout, std::function<std::function<void(SubscriptionEvent)>(Status, std::string, std::chrono::seconds)> cb)
{
    if (!m_eventServer)
    {
        throw std::runtime_error("UPnP library is not properly initialized");
    }

    auto addr = m_eventServer->getAddress();
    auto eventServerUrl = fmt::format("http://{}:{}/", addr.ip(), addr.port());

    uv::asyncSend(*m_loop, [=] () {
#ifdef DEBUG_UPNP_CLIENT
        log::debug("Subscribe to service: {}", publisherUrl);
#endif
        m_httpClient->subscribe(publisherUrl, eventServerUrl, timeout, [this, cb] (int32_t status, std::string subId, std::chrono::seconds subTimeout, std::string response) {
            //log::debug("Subscribe response: {}", response);

            if (cb)
            {
                auto subCb = cb(httpStatusToStatus(status), subId, subTimeout);
                if (subCb)
                {
                    m_eventCallbacks.emplace(subId, subCb);
                }
            }
        });
    });
}

void Client2::renewSubscription(const std::string& publisherUrl,
                                const std::string& subscriptionId,
                                std::chrono::seconds timeout,
                                std::function<void(Status status, std::string subId, std::chrono::seconds timeout)> cb)
{
    assert(m_eventServer);
    assert(timeout.count() > 0);

    uv::asyncSend(*m_loop, [=] () {
#ifdef DEBUG_UPNP_CLIENT
        log::debug("Renew subscription: {} {}", publisherUrl, subscriptionId);
#endif
        m_httpClient->renewSubscription(publisherUrl, subscriptionId, timeout, [this, cb] (int32_t status, std::string subId, std::chrono::seconds subTimeout, std::string response) {
            //log::debug("Subscription renewal response: {}", response);

            if (cb)
            {
                cb(httpStatusToStatus(status), subId, subTimeout);
            }
        });
    });
}

void Client2::unsubscribeFromService(const std::string& publisherUrl, const std::string& subscriptionId, std::function<void(Status status)> cb)
{
    uv::asyncSend(*m_loop, [=] () {
        m_httpClient->unsubscribe(publisherUrl, subscriptionId, [=] (int32_t status, std::string response) {
//#ifdef DEBUG_UPNP_CLIENT
            log::debug("Unsubscribe response: {}", response);
//#endif
            if (cb)
            {
                cb(httpStatusToStatus(status));
            }

            m_eventCallbacks.erase(subscriptionId);
        });
    });
}

void Client2::sendAction(const Action& action, std::function<void(Status, std::string)> cb)
{
#ifdef DEBUG_UPNP_CLIENT
    log::debug("Execute action: {}", action.getActionDocument().toString());
#endif

    uv::asyncSend(*m_loop, [this, url = action.getUrl(), name = action.getName(), urn = action.getServiceTypeUrn(), env = action.toString(), cb = std::move(cb)] () {
        m_httpClient->soapAction(url, name, urn, env, [cb] (int32_t status, std::string response) {
            cb(httpStatusToStatus(status), std::move(response));
        });
    });

#ifdef DEBUG_UPNP_CLIENT
    log::debug(result.toString());
#endif
}

void Client2::getFile(const std::string& url, std::function<void(Status, std::string contents)> cb)
{
    m_httpClient->get(url, [cb] (int32_t status, std::string contents) {
        cb(httpStatusToStatus(status), std::move(contents));
    });
}

uv::Loop& Client2::loop() const
{
    return *m_loop;
}

void Client2::handlEvent(const SubscriptionEvent& event)
{
    auto iter = m_eventCallbacks.find(event.sid);
    if (iter != m_eventCallbacks.end())
    {
        iter->second(event);
    }
}

}
