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

#include <array>
#include <string>
#include <unordered_map>

#include "upnp/upnp.uv.h"
#include "upnp/upnp.http.parser.h"

namespace upnp
{
namespace http
{

using RequestCb = std::function<std::string(http::Parser&)>;

class Server
{
public:
    Server(uv::Loop& loop, const uv::Address& address);

    void stop(std::function<void()> cb);

    void addFile(const std::string& urlPath, const std::string& contentType, const std::string& contents);
    std::string getWebRootUrl() const;
    uv::Address getAddress() const;

    void setRequestHandler(Method method, RequestCb cb);

private:
    struct HostedFile
    {
        std::string contentType;
        std::string data;
    };

    void writeResponse(uv::socket::Tcp* client, const std::string& response, bool closeConnection);
    void writeResponse(uv::socket::Tcp* client, const std::string& header, const std::string& body, bool closeConnection);
    void cleanupClient(uv::socket::Tcp* client) noexcept;

    void onHttpParseCompleted(std::shared_ptr<http::Parser> parser, uv::socket::Tcp* client);

    uv::Loop& m_loop;
    uv::socket::Tcp m_socket;
    std::unordered_map<std::string, HostedFile> m_serverdFiles;
    std::unordered_map<void*, std::unique_ptr<uv::socket::Tcp>> m_clients;

    std::array<RequestCb, std::underlying_type_t<Method>(Method::Unknown)> m_handlers;
};

}
}
