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

#include "upnp.clienttestbase.h"
#include "upnp/upnp.avtransport.client.h"

namespace upnp
{

namespace AVTransport
{

bool operator==(const TransportInfo& lhs, const TransportInfo& rhs)
{
    return    lhs.currentTransportState == rhs.currentTransportState
           && lhs.currentTransportStatus == rhs.currentTransportStatus
           && lhs.currentSpeed == rhs.currentSpeed;
}

bool operator==(const PositionInfo& lhs, const PositionInfo& rhs)
{
    return    lhs.absoluteCount == rhs.absoluteCount
           && lhs.relativeCount == rhs.relativeCount
           && lhs.track == rhs.track
           && lhs.absoluteTime == rhs.absoluteTime
           && lhs.relativeTime == rhs.relativeTime
           && lhs.trackDuration == rhs.trackDuration
           && lhs.trackMetaData == rhs.trackMetaData
           && lhs.trackURI == rhs.trackURI;
}

bool operator==(const MediaInfo& lhs, const MediaInfo& rhs)
{
    return    lhs.numberOfTracks == rhs.numberOfTracks
           && lhs.mediaDuration == rhs.mediaDuration
           && lhs.currentURI == rhs.currentURI
           && lhs.currentURIMetaData == rhs.currentURIMetaData
           && lhs.nextURI == rhs.nextURI
           && lhs.nextURIMetaData == rhs.nextURIMetaData
           && lhs.playMedium == rhs.playMedium
           && lhs.recordMedium == rhs.recordMedium
           && lhs.writeStatus == rhs.writeStatus;
}

}

namespace test
{

namespace
{

const std::string s_eventNameSpaceId = "AVS";

struct AVTransportStatusCallbackMock
{
    MOCK_METHOD1(onStatus, void(Status));
    MOCK_METHOD2(onStatus, void(Status, AVTransport::TransportInfo info));
    MOCK_METHOD2(onStatus, void(Status, AVTransport::PositionInfo info));
    MOCK_METHOD2(onStatus, void(Status, AVTransport::MediaInfo info));
    MOCK_METHOD2(onStatus, void(Status, std::set<AVTransport::Action>));
};

}

class AVTransportClientTest : public ServiceClientTestBase<AVTransport::Client, AVTransportStatusCallbackMock, AVTransport::Variable>
{
public:
    AVTransportClientTest() : ServiceClientTestBase(testxmls::avtransportServiceDescription)
    {
    }

    void triggerLastChangeUpdate()
    {
        std::vector<testxmls::EventValue> ev = { { "TransportState", "PLAYING" },
                                                 { "TransportStatus", "OK" },
                                                 { "PlaybackStorageMedium", "NETWORK" },
                                                 { "CurrentTrackDuration", "01:05" },
                                                 { "AVTransportURI", "http://someurl.mp3" },
                                                 { "PossiblePlaybackStorageMedia", "media" },
                                                 { "CurrentPlayMode", "NORMAL" },
                                                 { "TransportPlaySpeed", "1"},
                                                 { "RecordMediumWriteStatus", "NOT_IMPLEMENTED" },
                                                 { "RecordStorageMedium", "NOT_IMPLEMENTED" },
                                                 { "CurrentRecordQualityMode", "NOT_IMPLEMENTED" },
                                                 { "PossibleRecordQualityModes", "NOT_IMPLEMENTED" },
                                                 { "NumberOfTracks", "1" },
                                                 { "CurrentTrack", "1" },
                                                 { "CurrentMediaDuration", "01:06" },
                                                 { "CurrentTrackMetaData", "Metadata" },
                                                 { "CurrentTrackURI", "http://trackurl.mp3" },
                                                 { "AVTransportURIMetaData", "AVmetadata" },
                                                 { "NextAVTransportURI", "NOT_IMPLEMENTED" },
                                                 { "NextAVTransportURIMetaData", "NOT_IMPLEMENTED" },
                                                 { "RelativeTimePosition", "00:30" },
                                                 { "AbsoluteTimePosition", "00:10" },
                                                 { "RelativeCounterPosition", "4" },
                                                 { "AbsoluteCounterPosition", "1" },
                                                 { "CurrentTransportActions", "Prev,Next,Stop" },
                                                 { "A_ARG_TYPE_SeekMode", "TRACK_NR" },
                                                 { "A_ARG_TYPE_SeekTarget", "target" },
                                                 { "A_ARG_TYPE_InstanceID", "InstanceId" } };

        SubscriptionEvent event;
        event.sid  = s_subscriptionId;
        event.data = testxmls::generateStateVariableChangeEvent("LastChange", s_eventNameSpaceId, ev);
        eventCb(event);
    }
};

TEST_F(AVTransportClientTest, supportedActions)
{
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::GetCurrentTransportActions));
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::GetDeviceCapabilities));
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::GetMediaInfo));
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::GetPositionInfo));
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::GetTransportInfo));
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::GetTransportSettings));
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::Next));
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::Pause));
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::Play));
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::Previous));
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::Seek));
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::SetAVTransportURI));
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::SetPlayMode));
    EXPECT_TRUE(serviceInstance->supportsAction(AVTransport::Action::Stop));

    EXPECT_FALSE(serviceInstance->supportsAction(AVTransport::Action::Record));
    EXPECT_FALSE(serviceInstance->supportsAction(AVTransport::Action::SetRecordQualityMode));
    EXPECT_FALSE(serviceInstance->supportsAction(AVTransport::Action::SetNextAVTransportURI));
}

TEST_F(AVTransportClientTest, lastChangeEvent)
{
    std::map<AVTransport::Variable, std::string> lastChange;
    EXPECT_CALL(eventListener, LastChangedEvent(AVTransport::Variable::LastChange, _))
        .WillOnce(SaveArg<1>(&lastChange));

    triggerLastChangeUpdate();

    EXPECT_EQ("PLAYING",            lastChange[AVTransport::Variable::TransportState]);
    EXPECT_EQ("OK",                 lastChange[AVTransport::Variable::TransportStatus]);
    EXPECT_EQ("NETWORK",            lastChange[AVTransport::Variable::PlaybackStorageMedium]);
    EXPECT_EQ("01:05",              lastChange[AVTransport::Variable::CurrentTrackDuration]);
    EXPECT_EQ("http://someurl.mp3", lastChange[AVTransport::Variable::AVTransportURI]);
    EXPECT_EQ("media",              lastChange[AVTransport::Variable::PossiblePlaybackStorageMedia]);
    EXPECT_EQ("NORMAL",             lastChange[AVTransport::Variable::CurrentPlayMode]);
    EXPECT_EQ("1",                  lastChange[AVTransport::Variable::TransportPlaySpeed]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransport::Variable::RecordMediumWriteStatus]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransport::Variable::RecordStorageMedium]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransport::Variable::CurrentRecordQualityMode]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransport::Variable::PossibleRecordQualityModes]);
    EXPECT_EQ("1",                  lastChange[AVTransport::Variable::NumberOfTracks]);
    EXPECT_EQ("1",                  lastChange[AVTransport::Variable::CurrentTrack]);
    EXPECT_EQ("01:06",              lastChange[AVTransport::Variable::CurrentMediaDuration]);
    EXPECT_EQ("Metadata",           lastChange[AVTransport::Variable::CurrentTrackMetaData]);
    EXPECT_EQ("http://trackurl.mp3",lastChange[AVTransport::Variable::CurrentTrackURI]);
    EXPECT_EQ("AVmetadata",         lastChange[AVTransport::Variable::AVTransportURIMetaData]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransport::Variable::NextAVTransportURI]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransport::Variable::NextAVTransportURIMetaData]);
    EXPECT_EQ("00:30",              lastChange[AVTransport::Variable::RelativeTimePosition]);
    EXPECT_EQ("00:10",              lastChange[AVTransport::Variable::AbsoluteTimePosition]);
    EXPECT_EQ("4",                  lastChange[AVTransport::Variable::RelativeCounterPosition]);
    EXPECT_EQ("1",                  lastChange[AVTransport::Variable::AbsoluteCounterPosition]);
    EXPECT_EQ("Prev,Next,Stop",     lastChange[AVTransport::Variable::CurrentTransportActions]);
    EXPECT_EQ("TRACK_NR",           lastChange[AVTransport::Variable::ArgumentTypeSeekMode]);
    EXPECT_EQ("target",             lastChange[AVTransport::Variable::ArgumentTypeSeekTarget]);
    EXPECT_EQ("InstanceId",         lastChange[AVTransport::Variable::ArgumentTypeInstanceId]);
}

TEST_F(AVTransportClientTest, play)
{
    Action expectedAction("Play", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));
    expectedAction.addArgument("Speed", "2");

    expectAction(expectedAction);
    EXPECT_CALL(statusMock, onStatus(Status()));
    serviceInstance->play(s_connectionId, "2", checkStatusCallback());
}

TEST_F(AVTransportClientTest, playCoro)
{
    Action expectedAction("Play", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));
    expectedAction.addArgument("Speed", "2");

    EXPECT_CALL(client, sendAction(_)).WillOnce(Invoke([&] (auto& action) -> Future<soap::ActionResult> {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        co_return wrapSoap(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType(), {}));
    }));

    serviceInstance->play(s_connectionId, "2").get();
}

TEST_F(AVTransportClientTest, stop)
{
    Action expectedAction("Stop", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    expectAction(expectedAction);
    EXPECT_CALL(statusMock, onStatus(Status()));
    serviceInstance->stop(s_connectionId, checkStatusCallback());
}

TEST_F(AVTransportClientTest, stopCoro)
{
    Action expectedAction("Stop", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    expectActionCoro(expectedAction);
    serviceInstance->stop(s_connectionId).get();
}

TEST_F(AVTransportClientTest, pause)
{
    Action expectedAction("Pause", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    expectAction(expectedAction);
    EXPECT_CALL(statusMock, onStatus(Status()));
    serviceInstance->pause(s_connectionId, checkStatusCallback());
}

TEST_F(AVTransportClientTest, pauseCoro)
{
    Action expectedAction("Pause", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    expectActionCoro(expectedAction);
    serviceInstance->pause(s_connectionId);
}

TEST_F(AVTransportClientTest, previous)
{
    Action expectedAction("Previous", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    expectAction(expectedAction);
    EXPECT_CALL(statusMock, onStatus(Status()));
    serviceInstance->previous(s_connectionId, checkStatusCallback());
}

TEST_F(AVTransportClientTest, previousCoro)
{
    Action expectedAction("Previous", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    expectActionCoro(expectedAction);
    serviceInstance->previous(s_connectionId);
}

TEST_F(AVTransportClientTest, next)
{
    Action expectedAction("Next", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    expectAction(expectedAction);
    EXPECT_CALL(statusMock, onStatus(Status()));
    serviceInstance->next(s_connectionId, checkStatusCallback());
}

TEST_F(AVTransportClientTest, nextCoro)
{
    Action expectedAction("Next", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    expectActionCoro(expectedAction);
    serviceInstance->next(s_connectionId);
}

TEST_F(AVTransportClientTest, getTransportInfo)
{
    Action expectedAction("GetTransportInfo", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    expectAction(expectedAction, { { "CurrentTransportState",  "PLAYING" },
                                   { "CurrentTransportStatus", "OK" },
                                   { "CurrentSpeed",           "Speed"} });

    AVTransport::TransportInfo expectedInfoResponse;
    expectedInfoResponse.currentTransportState = AVTransport::State::Playing;
    expectedInfoResponse.currentTransportStatus = AVTransport::Status::Ok;
    expectedInfoResponse.currentSpeed = "Speed";

    EXPECT_CALL(statusMock, onStatus(Status(), expectedInfoResponse));
    serviceInstance->getTransportInfo(s_connectionId, checkStatusCallback<AVTransport::TransportInfo>());
}

TEST_F(AVTransportClientTest, getTransportInfoCoro)
{
    Action expectedAction("GetTransportInfo", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    std::vector<std::pair<std::string, std::string>> responseVars = {
        { "CurrentTransportState",  "PLAYING" },
        { "CurrentTransportStatus", "OK" },
        { "CurrentSpeed",           "Speed"}
    };

    expectActionCoroResponse(expectedAction, responseVars);

    AVTransport::TransportInfo expectedInfoResponse;
    expectedInfoResponse.currentTransportState = AVTransport::State::Playing;
    expectedInfoResponse.currentTransportStatus = AVTransport::Status::Ok;
    expectedInfoResponse.currentSpeed = "Speed";

    EXPECT_EQ(expectedInfoResponse, serviceInstance->getTransportInfo(s_connectionId).get());
}

TEST_F(AVTransportClientTest, getPositionInfo)
{
    Action expectedAction("GetPositionInfo", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    expectAction(expectedAction, { { "AbsCount",      "1" },
                                   { "AbsTime",       "AbsTime" },
                                   { "RelTime",       "RelTime" },
                                   { "RelCount",      "2" },
                                   { "Track",         "3" },
                                   { "TrackDuration", "Duration" },
                                   { "TrackMetaData", "Meta" },
                                   { "TrackURI",      "URI"} } );

    AVTransport::PositionInfo expectedInfoResponse;
    expectedInfoResponse.absoluteCount = 1;
    expectedInfoResponse.relativeCount = 2;
    expectedInfoResponse.track = 3;
    expectedInfoResponse.absoluteTime = "AbsTime";
    expectedInfoResponse.relativeTime = "RelTime";
    expectedInfoResponse.trackDuration = "Duration";
    expectedInfoResponse.trackMetaData = "Meta";
    expectedInfoResponse.trackURI = "URI";

    EXPECT_CALL(statusMock, onStatus(Status(), expectedInfoResponse));
    serviceInstance->getPositionInfo(s_connectionId, checkStatusCallback<AVTransport::PositionInfo>());
}

TEST_F(AVTransportClientTest, getPositionInfoCoro)
{
    Action expectedAction("GetPositionInfo", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    std::vector<std::pair<std::string, std::string>> responseVars = {
        { "AbsCount",      "1" },
        { "AbsTime",       "AbsTime" },
        { "RelTime",       "RelTime" },
        { "RelCount",      "2" },
        { "Track",         "3" },
        { "TrackDuration", "Duration" },
        { "TrackMetaData", "Meta" },
        { "TrackURI",      "URI"}
    };

    expectActionCoroResponse(expectedAction, responseVars);

    AVTransport::PositionInfo expectedInfoResponse;
    expectedInfoResponse.absoluteCount = 1;
    expectedInfoResponse.relativeCount = 2;
    expectedInfoResponse.track = 3;
    expectedInfoResponse.absoluteTime = "AbsTime";
    expectedInfoResponse.relativeTime = "RelTime";
    expectedInfoResponse.trackDuration = "Duration";
    expectedInfoResponse.trackMetaData = "Meta";
    expectedInfoResponse.trackURI = "URI";

    EXPECT_EQ(expectedInfoResponse, serviceInstance->getPositionInfo(s_connectionId).get());
}

TEST_F(AVTransportClientTest, getMediaInfo)
{
    Action expectedAction("GetMediaInfo", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    expectAction(expectedAction, { { "NrTracks",            "5" },
                                   { "MediaDuration",       "Duration" },
                                   { "CurrentUri",          "Uri" },
                                   { "CurrentUriMetaData",  "UriMeta" },
                                   { "NextURI",             "NextUri" },
                                   { "NextURIMetaData",     "NextUriMeta" },
                                   { "PlayMedium",          "Medium" },
                                   { "RecordMedium",        "MediumRec"},
                                   { "WriteStatus",         "Status"} } );

    AVTransport::MediaInfo expectedInfoResponse;
    expectedInfoResponse.numberOfTracks = 5;
    expectedInfoResponse.mediaDuration = "Duration";
    expectedInfoResponse.currentURI = "Uri";
    expectedInfoResponse.currentURIMetaData = "UriMeta";
    expectedInfoResponse.nextURI = "NextUri";
    expectedInfoResponse.nextURIMetaData = "NextUriMeta";
    expectedInfoResponse.playMedium = "Medium";
    expectedInfoResponse.recordMedium = "MediumRec";
    expectedInfoResponse.writeStatus = "Status";

    EXPECT_CALL(statusMock, onStatus(Status(), expectedInfoResponse));
    serviceInstance->getMediaInfo(s_connectionId, checkStatusCallback<AVTransport::MediaInfo>());
}

TEST_F(AVTransportClientTest, getMediaInfoCoro)
{
    Action expectedAction("GetMediaInfo", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    std::vector<std::pair<std::string, std::string>> responseVars = {
        { "NrTracks",            "5" },
        { "MediaDuration",       "Duration" },
        { "CurrentUri",          "Uri" },
        { "CurrentUriMetaData",  "UriMeta" },
        { "NextURI",             "NextUri" },
        { "NextURIMetaData",     "NextUriMeta" },
        { "PlayMedium",          "Medium" },
        { "RecordMedium",        "MediumRec"},
        { "WriteStatus",         "Status"}
    };

    expectActionCoroResponse(expectedAction, responseVars);

    AVTransport::MediaInfo expectedInfoResponse;
    expectedInfoResponse.numberOfTracks = 5;
    expectedInfoResponse.mediaDuration = "Duration";
    expectedInfoResponse.currentURI = "Uri";
    expectedInfoResponse.currentURIMetaData = "UriMeta";
    expectedInfoResponse.nextURI = "NextUri";
    expectedInfoResponse.nextURIMetaData = "NextUriMeta";
    expectedInfoResponse.playMedium = "Medium";
    expectedInfoResponse.recordMedium = "MediumRec";
    expectedInfoResponse.writeStatus = "Status";

    EXPECT_EQ(expectedInfoResponse, serviceInstance->getMediaInfo(s_connectionId).get());
}

TEST_F(AVTransportClientTest, getCurrentTransportActions)
{
    Action expectedAction("GetCurrentTransportActions", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    expectAction(expectedAction, { { "Actions", "Pause,Stop,Play" } } );

    std::set<AVTransport::Action> expected = { AVTransport::Action::Play, AVTransport::Action::Pause, AVTransport::Action::Stop };
    EXPECT_CALL(statusMock, onStatus(Status(), Matcher<std::set<AVTransport::Action>>(ContainerEq(expected))));
    serviceInstance->getCurrentTransportActions(s_connectionId, checkStatusCallback<std::set<AVTransport::Action>>());
}

TEST_F(AVTransportClientTest, getCurrentTransportActionsCoro)
{
    Action expectedAction("GetCurrentTransportActions", s_controlUrl, serviceType());
    expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

    std::vector<std::pair<std::string, std::string>> responseVars = { { "Actions", "Pause,Stop,Play" } };
    expectActionCoroResponse(expectedAction, responseVars);

    std::set<AVTransport::Action> expected = { AVTransport::Action::Play, AVTransport::Action::Pause, AVTransport::Action::Stop };
    EXPECT_EQ(expected, serviceInstance->getCurrentTransportActions(s_connectionId).get());
}

}
}
