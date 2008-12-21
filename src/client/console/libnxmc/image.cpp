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
static wxImageList s_imgObjectsNormal(32, 32);	// Normal object images
static wxImageList s_imgStatusSmall(16, 16);		// Small status/severity images


//
// Init image lists
//

void LIBNXMC_EXPORTABLE NXMCInitImageLists()
{
	// Small object icons
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

	// Small status/severity icons
	s_imgStatusSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallNormal")));
	s_imgStatusSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallWarning")));
	s_imgStatusSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallMinor")));
	s_imgStatusSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallMajor")));
	s_imgStatusSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallCritical")));
	s_imgStatusSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallUnknown")));
	s_imgStatusSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallUnmanaged")));
	s_imgStatusSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallDisabled")));
	s_imgStatusSmall.Add(wxXmlResource::Get()->LoadIcon(_T("icoSmallTesting")));
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
		case IMAGE_LIST_OBJECTS_NORMAL:
			return &s_imgObjectsNormal;
		case IMAGE_LIST_STATUS_SMALL:
			return &s_imgStatusSmall;
	}
	return NULL;
}


//
// Get copy of image list
//

wxImageList LIBNXMC_EXPORTABLE *NXMCGetImageListCopy(int list)
{
	wxImageList *newList, *origList;

	origList = NXMCGetImageList(list);
	if (origList != NULL)
	{
		int i, w, h, count;

		origList->GetSize(0, w, h);
		count = origList->GetImageCount();
		newList = new wxImageList(w, h, true, count);
		for(i = 0; i < count; i++)
			newList->Add(origList->GetIcon(i));
	}
	else
	{
		newList = NULL;
	}
	return newList;
}
