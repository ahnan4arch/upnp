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

#include "utils/signal.h"
#include "utils/log.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <future>

using namespace utils;
using namespace testing;

#include "upnp/upnpmediaserver.h"
#include "upnp/upnpitem.h"
#include "upnp/upnp.factory.h"

#include "testxmls.h"
#include "testutils.h"
#include "upnpclientmock.h"

namespace upnp
{
namespace test
{

class StatusMock
{
public:
    MOCK_METHOD1(onStatus, void(int32_t));
    MOCK_METHOD2(onStatus, void(int32_t, std::vector<Item>));
};

namespace
{

void addServiceToDevice(Device& dev, ServiceType type, const std::string& scpUrl, const std::string& controlUrl)
{
    Service svc;
    svc.m_type = type;
    svc.m_scpdUrl = scpUrl;
    svc.m_controlURL = controlUrl;

    dev.m_services.emplace(svc.m_type, svc);
}

}

class MediaServerTest : public Test
{
public:
    MediaServerTest()
    : m_server(m_client)
    , m_device(std::make_shared<Device>())
    {
        std::promise<int32_t> promise;
        auto fut = promise.get_future();

        addServiceToDevice(*m_device, ServiceType::ConnectionManager, "CMSCPUrl", "CMCurl");
        addServiceToDevice(*m_device, ServiceType::ContentDirectory, "CDSCPUrl", "CDCurl");

        EXPECT_CALL(m_client, getFile("CMSCPUrl", _)).WillOnce(InvokeArgument<1>(200, testxmls::connectionManagerServiceDescription));
        EXPECT_CALL(m_client, getFile("CDSCPUrl", _)).WillOnce(InvokeArgument<1>(200, testxmls::contentDirectoryServiceDescription));

        InSequence seq;
        Action searchCaps("GetSearchCapabilities", "CDCurl", ServiceType::ContentDirectory);
        Action sortCaps("GetSortCapabilities", "CDCurl", ServiceType::ContentDirectory);
        expectAction(searchCaps, { { "SearchCaps", "upnp:artist,dc:title" } });
        expectAction(sortCaps, { { "SortCaps", "upnp:artist,dc:title,upnp:genre" } });

        m_server.setDevice(m_device, [&] (int32_t status) {
            auto p = std::move(promise);
            p.set_value(status);
        });

        EXPECT_EQ(200, fut.get());
    }

    void expectAction(const Action& expected, const std::vector<std::pair<std::string, std::string>>& responseVars = {})
    {
        using namespace ContentDirectory;
        EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(Invoke([&, responseVars] (auto& action, auto& cb) {
            EXPECT_EQ(expected.toString(), action.toString());
            cb(200, wrapSoap(generateActionResponse(expected.getName(), expected.getServiceType(), responseVars)));
        }));
    }

    std::function<void(int32_t)> checkStatusCallback()
    {
        return [this] (int32_t status) { m_statusMock.onStatus(status); };
    }

    template <typename T>
    std::function<void(int32_t, const T&)> checkStatusCallback()
    {
        return [this] (int32_t status, const T& arg) { m_statusMock.onStatus(status, arg); };
    }

protected:
    Client2Mock                 m_client;
    StatusMock                  m_statusMock;

    MediaServer                 m_server;
    std::shared_ptr<Device>     m_device;
};

TEST_F(MediaServerTest, DiscoveredServices)
{
    EXPECT_FALSE(m_device->m_services.end() == m_device->m_services.find(upnp::ServiceType::ContentDirectory));
    EXPECT_FALSE(m_device->m_services.end() == m_device->m_services.find(upnp::ServiceType::ConnectionManager));
    EXPECT_TRUE(m_device->m_services.end() == m_device->m_services.find(upnp::ServiceType::AVTransport));
}

TEST_F(MediaServerTest, SearchCapabilities)
{
    EXPECT_TRUE(m_server.canSearchForProperty(Property::Title));
    EXPECT_TRUE(m_server.canSearchForProperty(Property::Artist));
    EXPECT_FALSE(m_server.canSearchForProperty(Property::All));
}

TEST_F(MediaServerTest, SortCapabilities)
{
    EXPECT_TRUE(m_server.canSortOnProperty(Property::Artist));
    EXPECT_TRUE(m_server.canSortOnProperty(Property::Title));
    EXPECT_TRUE(m_server.canSortOnProperty(Property::Genre));
    EXPECT_FALSE(m_server.canSortOnProperty(Property::All));
}

TEST_F(MediaServerTest, getAllInContainer)
{
    Action expectedAction("Browse", "CDCurl", ServiceType::ContentDirectory);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(200, wrapSoap(testxmls::browseResponseContainers));
    }));

    InSequence seq;
    EXPECT_CALL(m_statusMock, onStatus(200, Matcher<std::vector<Item>>(SizeIs(2))));
    EXPECT_CALL(m_statusMock, onStatus(200, Matcher<std::vector<Item>>(IsEmpty())));
    m_server.getAllInContainer(MediaServer::rootId, checkStatusCallback<std::vector<Item>>());
}

TEST_F(MediaServerTest, getAllInContainerNoResults)
{
    Action expectedAction("Browse", "CDCurl", ServiceType::ContentDirectory);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(WithArg<1>(Invoke([&] (auto& cb) {
        cb(200, generateBrowseResponse({}, {}));
    })));

    EXPECT_CALL(m_statusMock, onStatus(200, Matcher<std::vector<Item>>(IsEmpty())));
    m_server.getAllInContainer(MediaServer::rootId, checkStatusCallback<std::vector<Item>>());
}

TEST_F(MediaServerTest, getAllInContainerSortAscending)
{
    Action expectedAction("Browse", "CDCurl", ServiceType::ContentDirectory);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SortCriteria", "+dc:title");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(200, wrapSoap(testxmls::browseResponseContainers));
    }));

    InSequence seq;
    EXPECT_CALL(m_statusMock, onStatus(200, Matcher<std::vector<Item>>(SizeIs(2))));
    EXPECT_CALL(m_statusMock, onStatus(200, Matcher<std::vector<Item>>(IsEmpty())));
    m_server.getAllInContainer(MediaServer::rootId, 0, 0, Property::Title, MediaServer::SortMode::Ascending, checkStatusCallback<std::vector<Item>>());
}

TEST_F(MediaServerTest, getAllInContainerSortDescending)
{
    Action expectedAction("Browse", "CDCurl", ServiceType::ContentDirectory);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SortCriteria", "-upnp:genre");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(200, wrapSoap(testxmls::browseResponseContainers));
    }));

    InSequence seq;
    EXPECT_CALL(m_statusMock, onStatus(200, Matcher<std::vector<Item>>(SizeIs(2))));
    EXPECT_CALL(m_statusMock, onStatus(200, Matcher<std::vector<Item>>(IsEmpty())));
    m_server.getAllInContainer(MediaServer::rootId, 0, 0, Property::Genre, MediaServer::SortMode::Descending, checkStatusCallback<std::vector<Item>>());
}

TEST_F(MediaServerTest, SearchRootContainer)
{
    std::map<Property, std::string> criteria { {Property::Title, "Video"}, {Property::Artist, "Prince"} };

    Action expectedAction("Search", "CDCurl", ServiceType::ContentDirectory);
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SearchCriteria", "dc:title contains \"Video\" and upnp:artist contains \"Prince\"");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(200, generateBrowseResponse(generateContainers(2, "object.container"), {}));
    }));

    InSequence seq;
    EXPECT_CALL(m_statusMock, onStatus(200, Matcher<std::vector<Item>>(SizeIs(2))));
    EXPECT_CALL(m_statusMock, onStatus(200, Matcher<std::vector<Item>>(IsEmpty())));
    m_server.search(MediaServer::rootId, criteria, checkStatusCallback<std::vector<Item>>());
}

TEST_F(MediaServerTest, SearchRootContainerNoResults)
{
    std::map<Property, std::string> criteria { {Property::Title, "Video"}, {Property::Artist, "Prince"} };

    Action expectedAction("Search", "CDCurl", ServiceType::ContentDirectory);
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SearchCriteria", "dc:title contains \"Video\" and upnp:artist contains \"Prince\"");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(WithArg<1>(Invoke([&] (auto& cb) {
        cb(200, generateBrowseResponse({}, {}));
    })));

    InSequence seq;
    EXPECT_CALL(m_statusMock, onStatus(200, Matcher<std::vector<Item>>(IsEmpty())));
    m_server.search(MediaServer::rootId, criteria, checkStatusCallback<std::vector<Item>>());
}

TEST_F(MediaServerTest, SupportedActions)
{
    EXPECT_TRUE(m_server.connectionManager().supportsAction(ConnectionManager::Action::GetProtocolInfo));
    EXPECT_TRUE(m_server.connectionManager().supportsAction(ConnectionManager::Action::GetCurrentConnectionIDs));
    EXPECT_TRUE(m_server.connectionManager().supportsAction(ConnectionManager::Action::GetCurrentConnectionInfo));
    EXPECT_FALSE(m_server.connectionManager().supportsAction(ConnectionManager::Action::PrepareForConnection));
    EXPECT_FALSE(m_server.connectionManager().supportsAction(ConnectionManager::Action::ConnectionComplete));
}

TEST_F(MediaServerTest, SearchOnNotSupportedProperty)
{
    // title, artist are supported search properties (see test setup)
    std::map<Property, std::string> criteria { {Property::Title, "Video"}, {Property::Genre, "funk"} };
    EXPECT_THROW(m_server.search(MediaServer::rootId, criteria, checkStatusCallback<std::vector<Item>>()), std::runtime_error);
}

TEST_F(MediaServerTest, SortOnNotSupportedProperty)
{
    // title, artist, genre are supported search properties (see test setup)
    EXPECT_THROW(m_server.getAllInContainer(MediaServer::rootId, 0, 0, Property::Class, MediaServer::SortMode::Ascending, checkStatusCallback<std::vector<Item>>()), std::runtime_error);
}

}
}