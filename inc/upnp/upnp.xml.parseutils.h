#pragma once

#include "upnp/upnp.item.h"
#include "upnp/upnpstatevariable.h"
#include "upnp/upnp.contentdirectory.types.h"

#include <string>
#include <functional>
#include <string_view>


namespace rapidxml_ns
{

template<class Ch> class xml_node;
template<class Ch> class xml_document;

}

namespace upnp
{

struct Device;
class ServiceVariable;

namespace xml
{

std::string encode(std::string_view data);
std::string decode(std::string_view data);

void parseDeviceInfo(const std::string& xml, Device& device);
std::map<std::string, std::string> getEventValues(rapidxml_ns::xml_document<char>& doc);

Item parseContainer(rapidxml_ns::xml_node<char>& containerElem);
std::vector<Item> parseContainers(const std::string& xml);
Item parseItem(rapidxml_ns::xml_node<char>& itemElem);
Item parseItemDocument(const std::string& xml);
std::vector<Item> parseItems(const std::string& xml);
Item parseMetaData(const std::string& meta);
std::string parseBrowseResult(const std::string& response, ContentDirectory::ActionResult& result);
void parseEvent(const std::string& data, std::function<void(const std::string& varable, const std::map<std::string, std::string>&)> cb);
std::vector<StateVariable> parseServiceDescription(const std::string& contents, std::function<void(const std::string& action)> actionCb);

std::string createNotificationXml(const std::vector<std::pair<std::string, std::string>>& vars);

std::string optionalChildValue(const rapidxml_ns::xml_node<char>& node, const char* child);
std::string requiredChildValue(const rapidxml_ns::xml_node<char>& node, const char* child);

std::string createServiceEvent(uint32_t instanceId, const std::vector<ServiceVariable>& vars);
rapidxml_ns::xml_node<char>* serviceVariableToElement(rapidxml_ns::xml_document<char>& doc, const ServiceVariable& var);

std::string getItemDocument(const Item& item);
std::string getItemsDocument(const std::vector<Item>& items);

std::string toString(rapidxml_ns::xml_document<char>& doc);
std::string toString(rapidxml_ns::xml_node<char>& node);

template <typename T>
inline T optionalStringToUnsignedNumeric(const std::string& str)
{
    try
    {
        return str.empty() ? 0 : static_cast<T>(std::stoul(str));
    }
    catch (const std::invalid_argument&)
    {
        throw std::invalid_argument("Failed to convert string to integral: " + str);
    }
}

template <typename T>
inline T optionalStringToNumeric(const std::string& str)
{
    try
    {
        return str.empty() ? 0 : static_cast<T>(std::stol(str));
    }
    catch (const std::invalid_argument&)
    {
        throw std::invalid_argument("Failed to convert string to integral: " + str);
    }
}

}
}

