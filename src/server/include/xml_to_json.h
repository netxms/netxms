/*
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2025 Reden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: xml_to_json.h
**
**/


#ifndef _xml_to_json_h_
#define _xml_to_json_h_

#include <nxsrvapi.h>
#include <pugixml.h>

json_t LIBNXSRV_EXPORTABLE *XmlNodeToJson(const pugi::xml_node &node);


#endif   /* _xml_to_json_h_ */
