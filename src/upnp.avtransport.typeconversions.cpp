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


#include "upnp.avtransport.typeconversions.h"
#include "upnp.enumutils.h"

namespace upnp
{

using namespace AVTransport;

constexpr std::tuple<const char*, Action> s_actionNames[] {
    { "SetAVTransportURI",              Action::SetAVTransportURI },
    { "SetNextAVTransportURI",          Action::SetNextAVTransportURI },
    { "GetMediaInfo",                   Action::GetMediaInfo },
    { "GetTransportInfo",               Action::GetTransportInfo },
    { "GetPositionInfo",                Action::GetPositionInfo },
    { "GetDeviceCapabilities",          Action::GetDeviceCapabilities },
    { "GetTransportSettings",           Action::GetTransportSettings },
    { "Stop",                           Action::Stop },
    { "Play",                           Action::Play },
    { "Pause",                          Action::Pause },
    { "Record",                         Action::Record },
    { "Seek",                           Action::Seek },
    { "Next",                           Action::Next },
    { "Previous",                       Action::Previous },
    { "SetPlayMode",                    Action::SetPlayMode },
    { "SetRecordQualityMode",           Action::SetRecordQualityMode },
    { "GetCurrentTransportActions",     Action::GetCurrentTransportActions },

    { "GetMediaInfoExt",                Action::GetMediaInfoExt },
    { "GetDRMState",                    Action::GetDRMState },
    { "GetStateVariables",              Action::GetStateVariables },
    { "SetStateVariables",              Action::SetStateVariables },

    { "GetSyncOffset",                  Action::GetSyncOffset },
    { "AdjustSyncOffset",               Action::AdjustSyncOffset },
    { "SetSyncOffset",                  Action::SetSyncOffset },
    { "SyncPlay",                       Action::SyncPlay },
    { "SyncStop",                       Action::SyncStop },
    { "SyncPause",                      Action::SyncPause },
    { "SetStaticPlaylist",              Action::SetStaticPlaylist },
    { "SetStreamingPlaylist",           Action::SetStreamingPlaylist },
    { "GetPlaylistInfo",                Action::GetPlaylistInfo }
};

constexpr std::tuple<const char*, Variable> s_variableNames[] {
    { "TransportState",                        Variable::TransportState },
    { "TransportStatus",                       Variable::TransportStatus },
    { "PlaybackStorageMedium",                 Variable::PlaybackStorageMedium },
    { "PossiblePlaybackStorageMedia",          Variable::PossiblePlaybackStorageMedia },
    { "PossibleRecordStorageMedia",            Variable::PossibleRecordStorageMedia },
    { "CurrentPlayMode",                       Variable::CurrentPlayMode },
    { "TransportPlaySpeed",                    Variable::TransportPlaySpeed },
    { "RecordStorageMedium",                   Variable::RecordStorageMedium },
    { "RecordMediumWriteStatus",               Variable::RecordMediumWriteStatus },
    { "PossibleRecordQualityModes",            Variable::PossibleRecordQualityModes },
    { "CurrentRecordQualityMode",              Variable::CurrentRecordQualityMode },
    { "NumberOfTracks",                        Variable::NumberOfTracks },
    { "CurrentTrack",                          Variable::CurrentTrack },
    { "CurrentTrackDuration",                  Variable::CurrentTrackDuration },
    { "CurrentMediaDuration",                  Variable::CurrentMediaDuration },
    { "CurrentTrackURI",                       Variable::CurrentTrackURI },
    { "CurrentTrackMetaData",                  Variable::CurrentTrackMetaData },
    { "AVTransportURI",                        Variable::AVTransportURI },
    { "AVTransportURIMetaData",                Variable::AVTransportURIMetaData },
    { "NextAVTransportURI",                    Variable::NextAVTransportURI },
    { "NextAVTransportURIMetaData",            Variable::NextAVTransportURIMetaData },
    { "CurrentTransportActions",               Variable::CurrentTransportActions },
    { "RelativeTimePosition",                  Variable::RelativeTimePosition },
    { "AbsoluteTimePosition",                  Variable::AbsoluteTimePosition },
    { "RelativeCounterPosition",               Variable::RelativeCounterPosition },
    { "AbsoluteCounterPosition",               Variable::AbsoluteCounterPosition },
    { "A_ARG_TYPE_SeekMode",                   Variable::ArgumentTypeSeekMode },
    { "A_ARG_TYPE_SeekTarget",                 Variable::ArgumentTypeSeekTarget },
    { "A_ARG_TYPE_InstanceID",                 Variable::ArgumentTypeInstanceId },
    { "LastChange",                            Variable::LastChange },

    { "CurrentMediaCategory",                  Variable::CurrentMediaCategory },
    { "DRMState",                              Variable::DRMState },
    { "A_ARG_TYPE_DeviceUDN",                  Variable::ArgumentTypeDeviceUDN },
    { "A_ARG_TYPE_ServiceType",                Variable::ArgumentTypeServiceType },
    { "A_ARG_TYPE_ServiceId",                  Variable::ArgumentTypeServiceId },
    { "A_ARG_TYPE_StateVariableValuePairs",    Variable::ArgumentTypeStateVariableValuePairs },
    { "A_ARG_TYPE_StateVariableList",          Variable::ArgumentTypeStateVariableList },

    { "SyncOffset",                            Variable::SyncOffset },
    { "A_ARG_TYPE_PlaylistData",               Variable::ArgumentTypePlaylistData },
    { "A_ARG_TYPE_PlaylistDataLength",         Variable::ArgumentTypePlaylistDataLength },
    { "A_ARG_TYPE_PlaylistOffset",             Variable::ArgumentTypePlaylistOffset },
    { "A_ARG_TYPE_PlaylistTotalLength",        Variable::ArgumentTypePlaylistTotalLength },
    { "A_ARG_TYPE_PlaylistMIMEType",           Variable::ArgumentTypePlaylistMIMEType },
    { "A_ARG_TYPE_PlaylistExtendedType",       Variable::ArgumentTypePlaylistExtendedType },
    { "A_ARG_TYPE_PlaylistStep",               Variable::ArgumentTypePlaylistStep },
    { "A_ARG_TYPE_PlaylistType",               Variable::ArgumentTypePlaylistType },
    { "A_ARG_TYPE_PlaylistInfo",               Variable::ArgumentTypePlaylistInfo },
    { "A_ARG_TYPE_PlaylistStartObjID",         Variable::ArgumentTypePlaylistStartObjID }
};

constexpr std::tuple<const char*, State> s_stateNames[] {
    { "STOPPED",             State::Stopped },
    { "PLAYING",             State::Playing },
    { "TRANSITIONING",       State::Transitioning },
    { "PAUSED_PLAYBACK",     State::PausedPlayback },
    { "PAUSED_RECORDING",    State::PausedRecording },
    { "RECORDING",           State::Recording },
    { "NO_MEDIA_PRESENT",    State::NoMediaPresent }
};

constexpr std::tuple<const char*, PlaylistType> s_playlistTypeNames[] {
    {"Static",               PlaylistType::Static },
    {"StaticPIContents",     PlaylistType::StaticPIContents },
    {"Streaming",            PlaylistType::Streaming }
};

constexpr std::tuple<const char*, PlaylistState> s_playlistStateNames[] {
    { "Idle",           PlaylistState::Idle },
    { "Ready",          PlaylistState::Ready },
    { "Active",         PlaylistState::Active },
    { "Incomplete",     PlaylistState::Incomplete }
};

constexpr std::tuple<const char*, PlaylistStep> s_playlistStepNames[] {
    { "Initial",                    PlaylistStep::Initial },
    { "Continue",                   PlaylistStep::Continue },
    { "Stop",                       PlaylistStep::Stop },
    { "Reset",                      PlaylistStep::Reset },
    { "Replace",                    PlaylistStep::Replace }
};

constexpr std::tuple<const char*, CurrentMediaCategory> s_curMediaCategoryNames[] {
    { "NO_MEDIA",                   CurrentMediaCategory::NoMedia },
    { "TRACK_AWARE",                CurrentMediaCategory::TrackAware },
    { "TRACK_UNAWARE",              CurrentMediaCategory::TrackUnaware }
};

constexpr std::tuple<const char*, DRMState> s_drmStateNames[] {
    { "OK",                         DRMState::Ok },
    { "UNKNOWN",                    DRMState::DRMUnknown },
    { "PROCESSING_CONTENT_KEY",     DRMState::ProcessingContentKey },
    { "CONTENT_KEY_FAILURE",        DRMState::ContentKeyFailure },
    { "ATTEMPTING_AUTHENTICATION",  DRMState::AttemptingAuthentication },
    { "FAILED_AUTHENTICATION",      DRMState::FailedAuthentication },
    { "NOT_AUTHENTICATED",          DRMState::NotAuthenticated },
    { "DEVICE_REVOCATION",          DRMState::DeviceRevocation },
    { "DRM_SYSTEM_NOT_SUPPORTED",   DRMState::DrmSystemNotSupported },
    { "LICENSE_DENIED",             DRMState::LicenseDenied },
    { "LICENSE_EXPIRED",            DRMState::LicenseExpired },
    { "LICENSE_INSUFFICIENT",       DRMState::LicenseInsufficient }
};

constexpr std::tuple<const char*, Status> s_statusNames[] {
    { "OK",                         Status::Ok },
    { "ERROR_OCCURRED",             Status::Error }
};

constexpr std::tuple<const char*, PlayMode> s_playModeNames[] {
    { "NORMAL",           PlayMode::Normal },
    { "SHUFFLE",          PlayMode::Shuffle },
    { "REPEAT_ONE",       PlayMode::RepeatOne },
    { "REPEAT_ALL",       PlayMode::RepeatAll },
    { "RANDOM",           PlayMode::Random },
    { "DIRECT_1",         PlayMode::Direct },
    { "INTRO",            PlayMode::Intro }
};

constexpr std::tuple<const char*, SeekMode> s_seekModeNames[] {
    { "TRACK_NR",        SeekMode::TrackNumber },
    { "ABS_TIME",        SeekMode::AbsoluteTime },
    { "REL_TIME",        SeekMode::RelativeTime },
    { "ABS_COUNT",       SeekMode::AbsoluteCount },
    { "REL_COUNT",       SeekMode::RelativeCount },
    { "CHANNEL_FREQ",    SeekMode::ChannelFrequency },
    { "TAPE-INDEX",      SeekMode::TapeIndex },
    { "REL_TAPE-INDEX",  SeekMode::RelativeTapeIndex },
    { "FRAME",           SeekMode::Frame },
    { "REL_FRAME",       SeekMode::RelativeFrame }
};

ADD_ENUM_MAP(Action, s_actionNames)
ADD_ENUM_MAP(Variable, s_variableNames)
ADD_ENUM_MAP(State, s_stateNames)
ADD_ENUM_MAP(PlaylistType, s_playlistTypeNames)
ADD_ENUM_MAP(PlaylistState, s_playlistStateNames)
ADD_ENUM_MAP(PlaylistStep, s_playlistStepNames)
ADD_ENUM_MAP(CurrentMediaCategory, s_curMediaCategoryNames)
ADD_ENUM_MAP(DRMState, s_drmStateNames)
ADD_ENUM_MAP(Status, s_statusNames)
ADD_ENUM_MAP(PlayMode, s_playModeNames)
ADD_ENUM_MAP(SeekMode, s_seekModeNames)

Action AVTransport::actionFromString(gsl::span<const char> value)
{
    return enum_cast<Action>(value.data(), value.size());
}

const char* AVTransport::actionToString(Action value)
{
    return string_cast(value);
}

Variable AVTransport::variableFromString(gsl::span<const char> value)
{
    return enum_cast<Variable>(value.data(), value.size());
}

const char* AVTransport::variableToString(Variable value)
{
    return string_cast(value);
}

State AVTransport::stateFromString(gsl::span<const char> value)
{
    return enum_cast<State>(value.data(), value.size());
}

PlaylistType AVTransport::playlistTypeFromString(gsl::span<const char> value)
{
    return enum_cast<PlaylistType>(value.data(), value.size());
}

PlaylistStep AVTransport::playlistStepFromString(gsl::span<const char> value)
{
    return enum_cast<PlaylistStep>(value.data(), value.size());
}

const char* AVTransport::toString(State value)
{
    return string_cast(value);
}

const char* AVTransport::toString(CurrentMediaCategory value)
{
    return string_cast(value);
}

const char* AVTransport::toString(DRMState value)
{
    return string_cast(value);
}

const char* AVTransport::toString(PlaylistStep value)
{
    return string_cast(value);
}

const char* AVTransport::toString(PlaylistType value)
{
    return string_cast(value);
}

const char* AVTransport::toString(PlaylistState value)
{
    return string_cast(value);
}

Status AVTransport::statusFromString(gsl::span<const char> value)
{
    return enum_cast<Status>(value.data(), value.size());
}

const char* AVTransport::toString(Status value)
{
    return string_cast(value);
}

SeekMode AVTransport::seekModeFromString(gsl::span<const char> value)
{
    return enum_cast<SeekMode>(value.data(), value.size());
}

const char* AVTransport::toString(SeekMode value)
{
    return string_cast(value);
}

PlayMode AVTransport::playModeFromString(gsl::span<const char> value)
{
    return enum_cast<PlayMode>(value.data(), value.size());
}

const char* AVTransport::toString(PlayMode value)
{
    return string_cast(value);
}

}
