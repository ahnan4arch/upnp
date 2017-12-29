//    Copyright (C) 2013 Dirk Vanden Boer <dirk.vdb@gmail.com>
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

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <map>
#include <string>

#include "upnp/upnp.actionresponse.h"
#include "upnp/upnp.rootdeviceinterface.h"
#include "upnp/upnp.servicefaults.h"
#include "upnp/upnpservicevariable.h"
#include "utils/log.h"
#include "utils/stringoperations.h"

namespace upnp
{

template <typename VariableType>
class DeviceService
{
public:
    DeviceService(IRootDevice& dev, ServiceType type)
    : m_rootDevice(dev)
    , m_type(type)
    {
        m_variables.insert(std::make_pair(0, std::map<VariableType, ServiceVariable>()));
    }

    virtual ~DeviceService() = default;

    virtual ActionResponse onAction(const std::string& action, const std::string& request) = 0;
    virtual std::string    getSubscriptionResponse()                                       = 0;

    std::map<std::string, std::string> getVariables(uint32_t id) const
    {
        std::scoped_lock<std::mutex>       lock(m_mutex);
        std::map<std::string, std::string> vars;
        for (auto& var : m_variables.at(id))
        {
            vars.insert(std::make_pair(variableToString(var.first), var.second.getValue()));
        }

        return vars;
    }

    ServiceVariable getVariable(VariableType var) const
    {
        return getInstanceVariable(0, var);
    }

    ServiceVariable getInstanceVariable(uint32_t id, VariableType var) const
    {
        std::scoped_lock<std::mutex> lock(m_mutex);
        auto                         vars = m_variables.find(id);
        if (vars != m_variables.end())
        {
            auto v = vars->second.find(var);
            if (v != vars->second.end())
            {
                return v->second;
            }
        }

        return ServiceVariable();
    }

    void setVariable(VariableType var, const std::string& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_variables[0][var] = ServiceVariable(variableToString(var), value);
    }

    void setVariable(VariableType var, const std::string& value, const std::string& attrName, const std::string& attrValue)
    {
        ServiceVariable serviceVar(variableToString(var), value);
        serviceVar.addAttribute(attrName, attrValue);

        std::lock_guard<std::mutex> lock(m_mutex);
        m_variables[0][var] = serviceVar;
    }

    virtual void setInstanceVariable(uint32_t id, VariableType var, const std::string& value)
    {
        ServiceVariable serviceVar(variableToString(var), value);

        std::lock_guard<std::mutex> lock(m_mutex);
        m_variables[id][var] = serviceVar;
    }

    void setInstanceVariable(uint32_t id, VariableType var, const std::string& value, const std::string& attrName, const std::string& attrValue)
    {
        ServiceVariable serviceVar(variableToString(var), value);
        serviceVar.addAttribute(attrName, attrValue);

        std::lock_guard<std::mutex> lock(m_mutex);
        m_variables[id][var] = serviceVar;
    }

    //    template <typename T>
    //    typename std::enable_if<std::is_integral<T>::value, void>::type setVariable(VariableType var, const T& value)
    //    {
    //        m_variables[0][var] = std::to_string(value);
    //    }
    //
    //    template <typename T>
    //    typename std::enable_if<std::is_integral<T>::value, void>::type setInstanceVariable(uint32_t id, VariableType var, const T& value, const std::string& attrName, const std::string& attrValue)
    //    {
    //        ServiceVariable serviceVar(variableToString(var), value);
    //        serviceVar.addAttribute(attrName, attrValue);
    //
    //        m_variables[id][var] = serviceVar;
    //    }

protected:
    virtual const char* variableToString(VariableType type) const = 0;

    static std::vector<std::string> csvToVector(const std::string& csv)
    {
        auto vec = utils::stringops::tokenize(csv, ',');
        for (auto& str : vec)
        {
            utils::stringops::trim_in_place(str);
        }
        return vec;
    }

    static std::string vectorToCSV(const std::vector<std::string>& items)
    {
        std::stringstream ss;
        for (auto& item : items)
        {
            if (ss.tellp() > 0)
            {
                ss << ',';
            }

            ss << item;
        }

        return ss.str();
    }

    template <typename T>
    static std::string vectorToCSV(const std::vector<T>& items, std::function<std::string(T)> toStringFunc)
    {
        std::stringstream ss;
        for (auto& item : items)
        {
            if (ss.tellp() > 0)
            {
                ss << ',';
            }

            ss << toStringFunc(item);
        }

        return ss.str();
    }

    IRootDevice&                                                m_rootDevice;
    ServiceType                                                 m_type;
    std::map<uint32_t, std::map<VariableType, ServiceVariable>> m_variables;
    mutable std::mutex                                          m_mutex;
};

} // namespace upnp
