/* 
** NetXMS - Network Management System
** Portable management console - plugin API library
** Copyright (C) 2007 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: image.cpp
**
**/

#include "libnxmc.h"


//
// Static data
//

static wxImageList s_imgObjectsSmall(16, 16);	// Small object images


//
// Init image lists
//

void LIBNXMC_EXPORTABLE NXMCInitImageLists()
{
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallUnknown")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallSubnet")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallNode")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallInterface")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallNetwork")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallContainer")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallZone")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallServiceRoot")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallTemplate")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallTemplateGroup")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallTemplateRoot")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallNetworkService")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallVPNConnector")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallCondition")));
	s_imgObjectsSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallCluster")));
}


//
// Get image list
//

wxImageList LIBNXMC_EXPORTABLE *NXMCGetImageList(int list)
{
	switch(list)
	{
		case IMAGE_LIST_OBJECTS_SMALL:
			return &s_imgObjectsSmall;
	}
	return NULL;
}
