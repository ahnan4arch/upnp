
#pragma once

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "upnp/upnp.ssdp.client.h"
#include "upnp/upnp.http.parser.h"

#include <regex>
#include <string_view>

namespace upnp
{
namespace ssdp
{

inline void parseUSN(const std::string& usn, DeviceNotificationInfo& info)
{
    try
    {
        std::regex re(R"((uuid:[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})(?:::(\S*))?)");
        std::smatch match;
        if (std::regex_match(usn, match, re))
        {
            info.deviceId = match.str(1);

            if (match.size() > 2)
            {
                info.deviceType = match.str(2);
            }
        }
        else
        {
            throw std::runtime_error("Failed to parse USN");
        }
    }
    catch (const std::regex_error& e)
    {
        throw std::runtime_error(fmt::format("Failed to parse USN: {}", e.what()));
    }
}

inline uint32_t parseCacheControl(const std::string& cacheControl)
{
    try
    {
        std::regex re(R"(max-age=(\d+))");
        std::smatch match;
        if (std::regex_match(cacheControl, match, re))
        {
            return std::stoi(match.str(1));
        }

        throw std::runtime_error("Failed to parse Cache Control: " + cacheControl);
    }
    catch (const std::regex_error& e)
    {
        throw std::runtime_error(fmt::format("Failed to parse Cache Control: {} ({})", cacheControl, e.what()));
    }
}

inline NotificationType notificationTypeFromString(const char* type, size_t length)
{
    if (strncmp(type, "ssdp:alive", length) == 0)
    {
        return NotificationType::Alive;
    }
    else if (strncmp(type, "ssdp:byebye", length) == 0)
    {
        return NotificationType::ByeBye;
    }

    throw std::runtime_error("Invalid notification type: " + std::string(type, length));
}

inline NotificationType notificationTypeFromString(const std::string& str)
{
    return notificationTypeFromString(str.data(), str.size());
}

class Parser
{
public:
    Parser()
    : m_parser(http::Type::Both)
    {
        m_parser.setHeadersCompletedCallback([this] () { parseData(); });
    }

    void setHeaderParsedCallback(std::function<void(const DeviceNotificationInfo&)> cb) noexcept
    {
        m_cb = std::move(cb);
    }

    size_t parse(const char* data, size_t size)
    {
        if (size == 0)
        {
            return 0;
        }

        return m_parser.parse(data, size);
    }

    size_t parse(const std::string& data) noexcept
    {
        if (data.empty())
        {
            return 0;
        }

        return m_parser.parse(data);
    }

    void reset()
    {
        m_parser.reset();
    }

private:
    void parseData() noexcept
    {
        if (!m_cb)
        {
            return;
        }

        try
        {
            if (m_parser.getMethod() == http::Method::Search)
            {
                // ignore search requests
                return;
            }

            DeviceNotificationInfo info;
            parseUSN(m_parser.headerValue("USN"), info);
            info.location = m_parser.headerValue("LOCATION");

            if (m_parser.getMethod() == http::Method::Notify)
            {
                // spontaneous notify message
                info.type = notificationTypeFromString(m_parser.headerValue("NTS"));
                info.deviceType = m_parser.headerValue("NT");

                if (info.type == NotificationType::Alive)
                {
                    info.expirationTime = parseCacheControl(m_parser.headerValue("CACHE-CONTROL"));
                }
            }
            else
            {
                // response to a search
                if (m_parser.getStatus() != 200)
                {
                    utils::log::warn("Error status in search response: {}", m_parser.getStatus());
                    return;
                }

                // direct responses do not fill in the NTS, mark them as alive
                info.type = NotificationType::Alive;
                info.deviceType = m_parser.headerValue("ST");
                info.expirationTime = parseCacheControl(m_parser.headerValue("CACHE-CONTROL"));
            }

            m_cb(info);
        }
        catch (std::exception& e)
        {
            utils::log::warn("Failed to parse ssdp client http notification data: {}", e.what());
        }
    }

    http::Parser m_parser;
    std::function<void(const DeviceNotificationInfo&)> m_cb;
};

class SearchParser
{
public:
    SearchParser()
    : m_parser(http::Type::Both) // We also receive response broadcasts
    {
        m_parser.setHeadersCompletedCallback([this] () { parseData(); });
    }

    void setSearchRequestCallback(std::function<void(const std::string& host, const std::string& searchTarget, std::chrono::seconds, const asio::ip::udp::endpoint& addr)> cb) noexcept
    {
        m_cb = std::move(cb);
    }

    size_t parse(std::string_view data, const asio::ip::udp::endpoint& addr)
    {
        assert(!data.empty());
        m_address = addr;

        auto size = m_parser.parse(data.data(), data.size());
        if (m_parser.getFlags().isSet(http::Parser::Flag::ConnectionClose))
        {
            // some of the UDP search message have the connection close flag set
            // after that flag the parser will give errors, so reset
            reset();
        }

        return size;
    }

    void reset()
    {
        m_parser.reset();
    }

    bool isCompleted()
    {
        return m_parser.isCompleted();
    }

private:
    void parseData() noexcept
    {
        if (!m_cb)
        {
            return;
        }

        try
        {
            if (m_parser.getMethod() == http::Method::Search)
            {
                if (m_parser.headerValue("MAN") == "\"ssdp:discover\"")
                {
                    // mx value is only present for multicast search
                    // unicast search response should be sent as fast as possible
                    auto mx = m_parser.headerValue("MX");

                    // MX values bigger then 5 are reduced back to 5
                    auto delay = std::chrono::seconds(mx.empty() ? 0 : std::min(5, std::stoi(mx)));
                    m_cb(m_parser.headerValue("HOST"), m_parser.headerValue("ST"), delay, m_address);
                }
            }
        }
        catch (std::exception& e)
        {
            utils::log::warn("Failed to parse sddp server http notification data: {}", e.what());
            reset();
        }
    }

    http::Parser m_parser;
    asio::ip::udp::endpoint m_address;
    std::function<void(const std::string&, const std::string&, std::chrono::seconds, const asio::ip::udp::endpoint&)> m_cb;
};

}
}
