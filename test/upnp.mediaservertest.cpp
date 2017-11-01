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

#include "upnp/upnp.mediaserver.h"
#include "upnp/upnp.item.h"
#include "upnp/upnp.factory.h"

#include "testxmls.h"
#include "testutils.h"
#include "upnp.clientmock.h"

namespace upnp
{
namespace test
{

class StatusMock
{
public:
    MOCK_METHOD1(onStatus, void(Status));
    MOCK_METHOD2(onStatus, void(Status, std::vector<Item>));
};

namespace
{

void addServiceToDevice(Device& dev, ServiceType type, const std::string& scpUrl, const std::string& controlUrl)
{
    Service svc;
    svc.type = type;
    svc.scpdUrl = scpUrl;
    svc.controlURL = controlUrl;

    dev.services.emplace(type.type, svc);
}

}

class MediaServerTest : public Test
{
public:
    MediaServerTest()
    : m_server(m_client)
    , m_device(std::make_shared<Device>())
    , m_cdSvcType({ ServiceType::ContentDirectory, 1 })
    {
        std::promise<ErrorCode> promise;
        auto fut = promise.get_future();

        addServiceToDevice(*m_device, { ServiceType::ConnectionManager, 1 }, "CMSCPUrl", "CMCurl");
        addServiceToDevice(*m_device, m_cdSvcType, "CDSCPUrl", "CDCurl");

        EXPECT_CALL(m_client, getFile("CMSCPUrl")).WillOnce(InvokeWithoutArgs([] () -> Future<std::string> {
            co_return testxmls::connectionManagerServiceDescription;
        }));
        EXPECT_CALL(m_client, getFile("CDSCPUrl")).WillOnce(InvokeWithoutArgs([] () -> Future<std::string> {
            co_return testxmls::contentDirectoryServiceDescription;
        }));

        InSequence seq;
        Action searchCaps("GetSearchCapabilities", "CDCurl", m_cdSvcType);
        Action sortCaps("GetSortCapabilities", "CDCurl", m_cdSvcType);
        expectActionCoro(searchCaps, { { "SearchCaps", "upnp:artist,dc:title" } });
        expectActionCoro(sortCaps, { { "SortCaps", "upnp:artist,dc:title,upnp:genre" } });

        m_server.setDevice(m_device).get();
    }

    void expectAction(const Action& expected, const std::vector<std::pair<std::string, std::string>>& responseVars = {})
    {
        using namespace ContentDirectory;
        EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(Invoke([&, responseVars] (auto& action, auto& cb) {
            EXPECT_EQ(expected.toString(), action.toString());
            cb(Status(), wrapSoap(generateActionResponse(expected.getName(), expected.getServiceType(), responseVars)));
        }));
    }

    void expectActionCoro(const Action& expected, const std::vector<std::pair<std::string, std::string>>& responseVars = {})
    {
        EXPECT_CALL(m_client, sendAction(_)).WillOnce(Invoke([&expected, responseVars] (auto& action) -> Future<soap::ActionResult> {
            EXPECT_EQ(expected.toString(), action.toString());
            co_return wrapSoap(generateActionResponse(expected.getName(), expected.getServiceType(), responseVars));
        }));
    }

    std::vector<Item> getAllInContainer(const std::string& id)
    {
        auto fut = std::async(std::launch::any, [&] () -> Future<std::vector<Item>> {
            std::vector<Item> items;
            for co_await (const auto& item : m_server.getAllInContainer(id))
            {
                items.push_back(item);
                std::cout << item.getTitle() << std::endl;
            }

            co_return items;
        });

        return fut.get().get();
    }

    std::function<void(Status)> checkStatusCallback()
    {
        return [this] (Status status) { m_statusMock.onStatus(status); };
    }

    template <typename T>
    std::function<void(Status, const T&)> checkStatusCallback()
    {
        return [this] (Status status, const T& arg) { m_statusMock.onStatus(status, arg); };
    }

protected:
    ClientMock                  m_client;
    StatusMock                  m_statusMock;

    MediaServer                 m_server;
    std::shared_ptr<Device>     m_device;

    ServiceType                 m_cdSvcType;
};

TEST_F(MediaServerTest, DiscoveredServices)
{
    EXPECT_EQ(1, m_device->services.count(upnp::ServiceType::ContentDirectory));
    EXPECT_EQ(1, m_device->services.count(upnp::ServiceType::ConnectionManager));
    EXPECT_EQ(0, m_device->services.count(upnp::ServiceType::AVTransport));
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
    Action expectedAction("Browse", "CDCurl", m_cdSvcType);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(Status(), wrapSoap(testxmls::browseResponseContainers));
    }));

    InSequence seq;
    EXPECT_CALL(m_statusMock, onStatus(Status(), Matcher<std::vector<Item>>(SizeIs(2))));
    EXPECT_CALL(m_statusMock, onStatus(Status(), Matcher<std::vector<Item>>(IsEmpty())));
    m_server.getAllInContainer(MediaServer::rootId, checkStatusCallback<std::vector<Item>>());
}

TEST_F(MediaServerTest, getAllInContainerCoro)
{
    Action expectedAction("Browse", "CDCurl", m_cdSvcType);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_)).WillOnce(Invoke([&expectedAction] (auto& action) -> Future<soap::ActionResult> {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        co_return wrapSoap(testxmls::browseResponseContainers);
    }));

    EXPECT_EQ(2u, getAllInContainer(MediaServer::rootId).size());
}

TEST_F(MediaServerTest, getAllInContainerMultipleRequests)
{
    Action expectedAction1("Browse", "CDCurl", m_cdSvcType);
    expectedAction1.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction1.addArgument("Filter", "*");
    expectedAction1.addArgument("ObjectID", MediaServer::rootId);
    expectedAction1.addArgument("RequestedCount", "2");
    expectedAction1.addArgument("SortCriteria", "");
    expectedAction1.addArgument("StartingIndex", "0");

    Action expectedAction2("Browse", "CDCurl", m_cdSvcType);
    expectedAction2.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction2.addArgument("Filter", "*");
    expectedAction2.addArgument("ObjectID", MediaServer::rootId);
    expectedAction2.addArgument("RequestedCount", "2");
    expectedAction2.addArgument("SortCriteria", "");
    expectedAction2.addArgument("StartingIndex", "2");

    EXPECT_CALL(m_client, sendAction(_, _))
        .Times(2)
        .WillOnce(Invoke([&] (auto& action, auto& cb) {
            EXPECT_EQ(expectedAction1.toString(), action.toString());
            cb(Status(), wrapSoap(testxmls::browseResponseContainers));
        }))
        .WillOnce(Invoke([&] (auto& action, auto& cb) {
            EXPECT_EQ(expectedAction2.toString(), action.toString());
            cb(Status(), wrapSoap(testxmls::browseResponseContainersPart2));
        }));

    InSequence seq;
    EXPECT_CALL(m_statusMock, onStatus(Status(), Matcher<std::vector<Item>>(SizeIs(2))));
    EXPECT_CALL(m_statusMock, onStatus(Status(), Matcher<std::vector<Item>>(SizeIs(1))));
    EXPECT_CALL(m_statusMock, onStatus(Status(), Matcher<std::vector<Item>>(IsEmpty())));

    m_server.setRequestSize(2);
    m_server.getAllInContainer(MediaServer::rootId, checkStatusCallback<std::vector<Item>>());
}

TEST_F(MediaServerTest, getAllInContainerMultipleRequestsCoro)
{
    Action expectedAction1("Browse", "CDCurl", m_cdSvcType);
    expectedAction1.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction1.addArgument("Filter", "*");
    expectedAction1.addArgument("ObjectID", MediaServer::rootId);
    expectedAction1.addArgument("RequestedCount", "2");
    expectedAction1.addArgument("SortCriteria", "");
    expectedAction1.addArgument("StartingIndex", "0");

    Action expectedAction2("Browse", "CDCurl", m_cdSvcType);
    expectedAction2.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction2.addArgument("Filter", "*");
    expectedAction2.addArgument("ObjectID", MediaServer::rootId);
    expectedAction2.addArgument("RequestedCount", "2");
    expectedAction2.addArgument("SortCriteria", "");
    expectedAction2.addArgument("StartingIndex", "2");

    EXPECT_CALL(m_client, sendAction(_))
        .Times(2)
        .WillOnce(Invoke([&expectedAction1] (auto& action) -> Future<soap::ActionResult> {
            EXPECT_EQ(expectedAction1.toString(), action.toString());
            co_return wrapSoap(testxmls::browseResponseContainers);
        }))
        .WillOnce(Invoke([&expectedAction2] (auto& action) -> Future<soap::ActionResult> {
            EXPECT_EQ(expectedAction2.toString(), action.toString());
            co_return wrapSoap(testxmls::browseResponseContainersPart2);
        }));

    m_server.setRequestSize(2);
    EXPECT_EQ(3u, getAllInContainer(MediaServer::rootId).size());
}

TEST_F(MediaServerTest, getAllInContainerNoResults)
{
    Action expectedAction("Browse", "CDCurl", m_cdSvcType);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(WithArg<1>(Invoke([&] (auto& cb) {
        cb(Status(), generateBrowseResponse({}, {}));
    })));

    EXPECT_CALL(m_statusMock, onStatus(Status(), Matcher<std::vector<Item>>(IsEmpty())));
    m_server.getAllInContainer(MediaServer::rootId, checkStatusCallback<std::vector<Item>>());
}

TEST_F(MediaServerTest, getAllInContainerNoResultsCoro)
{
    Action expectedAction("Browse", "CDCurl", m_cdSvcType);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_)).WillOnce(Invoke([&expectedAction] (auto& action) -> Future<soap::ActionResult> {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        co_return generateBrowseResponse({}, {});
    }));

    EXPECT_TRUE(getAllInContainer(MediaServer::rootId).empty());
}

TEST_F(MediaServerTest, getAllInContainerSortAscending)
{
    Action expectedAction("Browse", "CDCurl", m_cdSvcType);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SortCriteria", "+dc:title");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(Status(), wrapSoap(testxmls::browseResponseContainers));
    }));

    InSequence seq;
    EXPECT_CALL(m_statusMock, onStatus(Status(), Matcher<std::vector<Item>>(SizeIs(2))));
    EXPECT_CALL(m_statusMock, onStatus(Status(), Matcher<std::vector<Item>>(IsEmpty())));
    m_server.getAllInContainer(MediaServer::rootId, 0, 0, Property::Title, MediaServer::SortMode::Ascending, checkStatusCallback<std::vector<Item>>());
}

TEST_F(MediaServerTest, getAllInContainerSortAscendingCoro)
{
    Action expectedAction("Browse", "CDCurl", m_cdSvcType);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SortCriteria", "+dc:title");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_)).WillOnce(Invoke([&expectedAction] (auto& action) -> Future<soap::ActionResult> {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        co_return wrapSoap(testxmls::browseResponseContainers);
    }));

    auto fut = std::async(std::launch::any, [&] () -> Future<std::vector<Item>> {
        std::vector<Item> items;
        for co_await (const auto& item : m_server.getAllInContainer(MediaServer::rootId, 0, 0, Property::Title, MediaServer::SortMode::Ascending))
        {
            items.push_back(item);
            std::cout << item.getTitle() << std::endl;
        }

        co_return items;
    });

    EXPECT_EQ(2u, fut.get().get().size());
}

TEST_F(MediaServerTest, getAllInContainerSortDescending)
{
    Action expectedAction("Browse", "CDCurl", m_cdSvcType);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SortCriteria", "-upnp:genre");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(Status(), wrapSoap(testxmls::browseResponseContainers));
    }));

    InSequence seq;
    EXPECT_CALL(m_statusMock, onStatus(Status(), Matcher<std::vector<Item>>(SizeIs(2))));
    EXPECT_CALL(m_statusMock, onStatus(Status(), Matcher<std::vector<Item>>(IsEmpty())));
    m_server.getAllInContainer(MediaServer::rootId, 0, 0, Property::Genre, MediaServer::SortMode::Descending, checkStatusCallback<std::vector<Item>>());
}

TEST_F(MediaServerTest, getAllInContainerSortDescendingCoro)
{
    Action expectedAction("Browse", "CDCurl", m_cdSvcType);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SortCriteria", "-upnp:genre");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_)).WillOnce(Invoke([&expectedAction] (auto& action) -> Future<soap::ActionResult> {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        co_return wrapSoap(testxmls::browseResponseContainers);
    }));

    auto fut = std::async(std::launch::any, [&] () -> Future<std::vector<Item>> {
        std::vector<Item> items;
        for co_await (const auto& item : m_server.getAllInContainer(MediaServer::rootId, 0, 0, Property::Genre, MediaServer::SortMode::Descending))
        {
            items.push_back(item);
            std::cout << item.getTitle() << std::endl;
        }

        co_return items;
    });

    EXPECT_EQ(2u, fut.get().get().size());
}

TEST_F(MediaServerTest, SearchRootContainer)
{
    std::map<Property, std::string> criteria { {Property::Title, "Video"}, {Property::Artist, "Prince"} };

    Action expectedAction("Search", "CDCurl", m_cdSvcType);
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SearchCriteria", "dc:title contains \"Video\" and upnp:artist contains \"Prince\"");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(Status(), generateBrowseResponse(generateContainers(2, "object.container"), {}));
    }));

    InSequence seq;
    EXPECT_CALL(m_statusMock, onStatus(Status(), Matcher<std::vector<Item>>(SizeIs(2))));
    EXPECT_CALL(m_statusMock, onStatus(Status(), Matcher<std::vector<Item>>(IsEmpty())));
    m_server.search(MediaServer::rootId, criteria, checkStatusCallback<std::vector<Item>>());
}

TEST_F(MediaServerTest, SearchRootContainerCoro)
{
    std::map<Property, std::string> criteria { {Property::Title, "Video"}, {Property::Artist, "Prince"} };

    Action expectedAction("Search", "CDCurl", m_cdSvcType);
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SearchCriteria", "dc:title contains \"Video\" and upnp:artist contains \"Prince\"");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_)).WillOnce(Invoke([&expectedAction] (auto& action) -> Future<soap::ActionResult> {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        co_return generateBrowseResponse(generateContainers(3, "object.container"), {});
    }));

    auto fut = std::async(std::launch::any, [&] () -> Future<std::vector<Item>> {
        std::vector<Item> items;
        for co_await (const auto& item : m_server.search(MediaServer::rootId, criteria))
        {
            items.push_back(item);
        }

        co_return items;
    });

    EXPECT_EQ(3u, fut.get().get().size());
}

TEST_F(MediaServerTest, SearchRootContainerNoResults)
{
    std::map<Property, std::string> criteria { {Property::Title, "Video"}, {Property::Artist, "Prince"} };

    Action expectedAction("Search", "CDCurl", m_cdSvcType);
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SearchCriteria", "dc:title contains \"Video\" and upnp:artist contains \"Prince\"");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(WithArg<1>(Invoke([&] (auto& cb) {
        cb(Status(), generateBrowseResponse({}, {}));
    })));

    InSequence seq;
    EXPECT_CALL(m_statusMock, onStatus(Status(), Matcher<std::vector<Item>>(IsEmpty())));
    m_server.search(MediaServer::rootId, criteria, checkStatusCallback<std::vector<Item>>());
}

TEST_F(MediaServerTest, SearchRootContainerNoResultsCoro)
{
    std::map<Property, std::string> criteria { {Property::Title, "Video"}, {Property::Artist, "Prince"} };

    Action expectedAction("Search", "CDCurl", m_cdSvcType);
    expectedAction.addArgument("Filter", "*");
    expectedAction.addArgument("ObjectID", MediaServer::rootId);
    expectedAction.addArgument("RequestedCount", "32");
    expectedAction.addArgument("SearchCriteria", "dc:title contains \"Video\" and upnp:artist contains \"Prince\"");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(m_client, sendAction(_)).WillOnce(Invoke([&expectedAction] (auto& action) -> Future<soap::ActionResult> {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        co_return generateBrowseResponse({}, {});
    }));

    auto fut = std::async(std::launch::any, [&] () -> Future<std::vector<Item>> {
        std::vector<Item> items;
        for co_await (const auto& item : m_server.search(MediaServer::rootId, criteria))
        {
            items.push_back(item);
            std::cout << item.getTitle() << std::endl;
        }

        co_return items;
    });

    EXPECT_TRUE(fut.get().get().empty());
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

TEST_F(MediaServerTest, SearchOnNotSupportedPropertyCoro)
{
    // title, artist are supported search properties (see test setup)
    std::map<Property, std::string> criteria { {Property::Title, "Video"}, {Property::Genre, "funk"} };
    EXPECT_THROW(m_server.search(MediaServer::rootId, criteria), std::runtime_error);
}

TEST_F(MediaServerTest, SortOnNotSupportedProperty)
{
    // title, artist, genre are supported search properties (see test setup)
    EXPECT_THROW(m_server.getAllInContainer(MediaServer::rootId, 0, 0, Property::Class, MediaServer::SortMode::Ascending, checkStatusCallback<std::vector<Item>>()), std::runtime_error);
}

TEST_F(MediaServerTest, SortOnNotSupportedPropertyCoro)
{
    // title, artist, genre are supported search properties (see test setup)
    auto fut = std::async(std::launch::any, [&] () -> Future<void> {
        for co_await (const auto& item : m_server.getAllInContainer(MediaServer::rootId, 0, 0, Property::Class, MediaServer::SortMode::Ascending))
        {
            std::cout << item.getTitle() << std::endl;
        }
        co_return;
    });

    EXPECT_THROW(fut.get().get(), std::runtime_error);
}

}
}
