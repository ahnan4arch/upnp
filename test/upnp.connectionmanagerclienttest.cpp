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

#include "upnp.serviceclienttestbase.h"
#include "upnp/upnp.connectionmanager.client.h"

using namespace utils;
using namespace testing;
using namespace std::placeholders;

#include "upnp/upnpxmlutils.h"

namespace upnp
{

namespace ConnectionManager
{

bool operator==(const ConnectionInfo& lhs, const ConnectionInfo& rhs)
{
    return    lhs.avTransportId == rhs.avTransportId
           && lhs.connectionId == rhs.connectionId
           && lhs.direction == rhs.direction
           && lhs.peerConnectionId == rhs.peerConnectionId
           && lhs.peerConnectionManager == rhs.peerConnectionManager
           && lhs.protocolInfo.toString() == rhs.protocolInfo.toString()
           && lhs.renderingControlServiceId == rhs.renderingControlServiceId
           && lhs.connectionStatus == rhs.connectionStatus;
}

}

bool operator==(const ProtocolInfo& lhs, const ProtocolInfo& rhs)
{
    return lhs.toString() == rhs.toString();
}

std::ostream& operator<<(std::ostream& os, const ProtocolInfo& lhs)
{
    return os << lhs.toString();
}

namespace test
{

static const std::string g_eventNameSpaceId = "CMS";

struct StatusCallbackMock
{
    MOCK_METHOD1(onStatus, void(int32_t));
    MOCK_METHOD2(onStatus, void(int32_t, std::vector<std::string>));
    MOCK_METHOD2(onStatus, void(int32_t, std::vector<ProtocolInfo>));
    MOCK_METHOD2(onStatus, void(int32_t, ConnectionManager::ConnectionInfo));
};

class ConnectionManagerClientTest : public ServiceClientTestBase<ConnectionManager::Client, StatusCallbackMock, ConnectionManager::Variable>
{
public:
    ConnectionManagerClientTest() : ServiceClientTestBase(ServiceType::ConnectionManager, testxmls::connectionManagerServiceDescription)
    {
    }

    void triggerLastChangeUpdate(const std::string& mute, const std::string& volume)
    {
        std::vector<testxmls::EventValue> ev = { { "PresetNameList", "FactoryDefaults, InstallationDefaults" },
                                                 { "Mute", mute, {{ "channel", "master" }} },
                                                 { "Volume", volume, {{ "channel", "master" }} } };

        SubscriptionEvent event;
        event.sid  = g_subscriptionId;
        event.data = testxmls::generateStateVariableChangeEvent("LastChange", g_eventNameSpaceId, ev);
        eventCb(event);
    }
};

TEST_F(ConnectionManagerClientTest, supportedActions)
{
    EXPECT_TRUE(serviceInstance->supportsAction(ConnectionManager::Action::GetProtocolInfo));
    EXPECT_TRUE(serviceInstance->supportsAction(ConnectionManager::Action::GetCurrentConnectionIDs));
    EXPECT_TRUE(serviceInstance->supportsAction(ConnectionManager::Action::GetCurrentConnectionInfo));

    EXPECT_FALSE(serviceInstance->supportsAction(ConnectionManager::Action::PrepareForConnection));
    EXPECT_FALSE(serviceInstance->supportsAction(ConnectionManager::Action::ConnectionComplete));
}

TEST_F(ConnectionManagerClientTest, getProtocolInfo)
{
    Action expectedAction("GetProtocolInfo", g_controlUrl, ServiceType::ConnectionManager);

    std::vector<ProtocolInfo> sinks = { ProtocolInfo("http-get:*:audio/mpeg:*"), ProtocolInfo("http-get:*:audio/wav:*") };
    expectAction(expectedAction, { { "Sink", "http-get:*:audio/mpeg:*,http-get:*:audio/wav:*" } });
    EXPECT_CALL(statusMock, onStatus(200, Matcher<std::vector<ProtocolInfo>>(ContainerEq(sinks))));
    serviceInstance->getProtocolInfo(checkStatusCallback<std::vector<ProtocolInfo>>());
}

TEST_F(ConnectionManagerClientTest, prepareForConnection)
{
    const ProtocolInfo protInfo("http-get:*:audio/mpeg:*");
    const std::string peerMgr = "ConnMgr";
    const int32_t peerConnectionId = 1;

    Action expectedAction("PrepareForConnection", g_controlUrl, ServiceType::ConnectionManager);
    expectedAction.addArgument("Direction", "Input");
    expectedAction.addArgument("PeerConnectionID", std::to_string(peerConnectionId));
    expectedAction.addArgument("PeerConnectionManager", peerMgr);
    expectedAction.addArgument("RemoteProtocolInfo", protInfo.toString());

    expectAction(expectedAction, { { "ConnectionId", std::to_string(g_connectionId) },
                                   { "AVTransportID", "5" },
                                   { "RcsID", "55" } } );

    ConnectionManager::ConnectionInfo expectedInfo;
    expectedInfo.avTransportId = 5;
    expectedInfo.connectionId = g_connectionId;
    expectedInfo.direction = ConnectionManager::Direction::Input;
    expectedInfo.peerConnectionId = peerConnectionId;
    expectedInfo.peerConnectionManager = peerMgr;
    expectedInfo.protocolInfo = ProtocolInfo("http-get:*:audio/mpeg:*");
    expectedInfo.renderingControlServiceId = 55;

    EXPECT_CALL(statusMock, onStatus(200, expectedInfo));
    serviceInstance->prepareForConnection(protInfo, peerMgr, peerConnectionId, ConnectionManager::Direction::Input, checkStatusCallback<ConnectionManager::ConnectionInfo>());
}

TEST_F(ConnectionManagerClientTest, connectionComplete)
{
    Action expectedAction("ConnectionComplete", g_controlUrl, ServiceType::ConnectionManager);
    expectedAction.addArgument("ConnectionID", std::to_string(g_connectionId));

    expectAction(expectedAction);
    
    ConnectionManager::ConnectionInfo connInfo;
    connInfo.connectionId = g_connectionId;

    EXPECT_CALL(statusMock, onStatus(200));
    serviceInstance->connectionComplete(connInfo, checkStatusCallback());
}

TEST_F(ConnectionManagerClientTest, getCurrentConnectionIds)
{
    Action expectedAction("GetCurrentConnectionIDs", g_controlUrl, ServiceType::ConnectionManager);

    expectAction(expectedAction, { { "ConnectionIDs", "3,5,6" } });
    
    std::vector<std::string> expectedIds = { "3", "5", "6" };
    EXPECT_CALL(statusMock, onStatus(200, Matcher<std::vector<std::string>>(ContainerEq(expectedIds))));
    serviceInstance->getCurrentConnectionIds(checkStatusCallback<std::vector<std::string>>());
}

TEST_F(ConnectionManagerClientTest, getCurrentConnectionInfo)
{
    const ProtocolInfo protInfo("http-get:*:audio/mpeg:*");
    const std::string peerMgr = "ConnMgr";
    const int32_t peerConnectionId = 1;

    Action expectedAction("GetCurrentConnectionInfo", g_controlUrl, ServiceType::ConnectionManager);
    expectedAction.addArgument("ConnectionID", std::to_string(g_connectionId));

    expectAction(expectedAction, { { "AVTransportID", "5" },
                                   { "RcsID", "55" },
                                   { "ProtocolInfo", protInfo.toString() },
                                   { "PeerConnectionManager", peerMgr },
                                   { "PeerConnectionID", std::to_string(peerConnectionId) },
                                   { "Direction", "Input" },
                                   { "Status", "OK" } });
    
    ConnectionManager::ConnectionInfo expectedInfo;
    expectedInfo.avTransportId = 5;
    expectedInfo.connectionId = g_connectionId;
    expectedInfo.direction = ConnectionManager::Direction::Input;
    expectedInfo.peerConnectionId = peerConnectionId;
    expectedInfo.peerConnectionManager = peerMgr;
    expectedInfo.protocolInfo = ProtocolInfo(protInfo);
    expectedInfo.renderingControlServiceId = 55;
    expectedInfo.connectionStatus = ConnectionManager::ConnectionStatus::Ok;
    
    EXPECT_CALL(statusMock, onStatus(200, expectedInfo));
    serviceInstance->getCurrentConnectionInfo(g_connectionId, checkStatusCallback<ConnectionManager::ConnectionInfo>());
}

}
}
