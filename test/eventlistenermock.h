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

#ifndef EVENT_LISTENER_MOCK_H
#define EVENT_LISTENER_MOCK_H

#include "gtest/gtest.h"

#include "upnp/upnpavtransportclient.h"
#include "upnp/upnprenderingcontrolclient.h"
#include "upnp/upnpcontentdirectoryclient.h"

namespace upnp
{
namespace test
{

class EventListenerMock
{
public:
    MOCK_METHOD2(RenderingControlLastChangedEvent, void(RenderingControl::Variable, const std::map<RenderingControl::Variable, std::string>&));
    MOCK_METHOD2(AVTransportLastChangedEvent, void(AVTransport::Variable, const std::map<AVTransport::Variable, std::string>&));
    MOCK_METHOD2(ContentDirectoryLastChangedEvent, void(ContentDirectory::Variable, const std::map<ContentDirectory::Variable, std::string>&));
};

}
}


#endif
