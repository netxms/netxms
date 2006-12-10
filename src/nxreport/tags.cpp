/*
** NetXMS - Network Management System
** Command line event sender
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: tags.cpp
**
**/

#include <nxclapi.h>
#include <stdarg.h>
#include "tags.h"

void print_tag(int nType, ...)
{
  va_list ap;
  char *c;
  int *i;
  DWORD d;
  
  va_start(ap, nType);
 
  switch (nType)
  {
  	case TAG_CONT_OPEN: 
  		c=va_arg(ap, char *);
  		d=va_arg(ap, DWORD);
  		_tprintf("<CONTAINER name='%s'>\n",c);
  		break;
	case TAG_CONT_CLOSE:
 		d=va_arg(ap, DWORD);	
		_tprintf("</CONTAINER>\n");
		break;
	case TAG_NODE_OPEN:
  		c=va_arg(ap, char *);
		_tprintf("<NODE name='%s'>\n",c);
		break;
	case TAG_NODE_CLOSE:
		_tprintf("</NODE>\n");
                break;
	case TAG_IFACE_OPEN:
  		c=va_arg(ap, char *);
		_tprintf("<IFACE name='%s'>\n",c);
		break;
	case TAG_IFACE_CLOSE:
		_tprintf("</IFACE>\n");
		break;
	case TAG_DCI_OPEN:
  		c=va_arg(ap, char *);
		_tprintf("<DCI name='%s'>\n",c);
		break;
	case TAG_DCI_CLOSE:
		_tprintf("</DCI>\n");	
		break; 
	case TAG_SERVICE_OPEN:
  		c=va_arg(ap, char *);
		_tprintf("<SERVICE name='%s'>\n",c);
		break;
	case TAG_SERVICE_CLOSE:
		_tprintf("</SERVICE>\n");	
		break; 
	case TAG_SUBNET_OPEN:
  		c=va_arg(ap, char *);
		_tprintf("<SUBNET name='%s'>\n",c);
		break;
	case TAG_SUBNET_CLOSE:
  		_tprintf("</SUBNET>\n");
		break;
	case TAG_NETW_OPEN:
  		c=va_arg(ap, char *);
		_tprintf("<NETWORK name='%s'>\n",c);
		break;
	case TAG_NETW_CLOSE:
  		_tprintf("</NETWORK>\n");
		break;	
	case TAG_ZONE_OPEN:
  		c=va_arg(ap, char *);
		_tprintf("<ZONE name='%s'>\n",c);
		break;
	case TAG_ZONE_CLOSE:
  		_tprintf("</ZONE>\n");
		break;		
	case TAG_SERVROOT_OPEN:
  		c=va_arg(ap, char *);
		_tprintf("<SERVICEROOT name='%s'>\n",c);
		break;
	case TAG_SERVROOT_CLOSE:
  		_tprintf("</SERVICEROOT>\n");
		break;
	case TAG_TEMPL_OPEN:
  		c=va_arg(ap, char *);
		_tprintf("<TEMPLATE name='%s'>\n",c);
		break;
	case TAG_TEMPL_CLOSE:
  		_tprintf("</TEMPLATE>\n");
		break;
	case TAG_TEMPLGRP_OPEN:
  		c=va_arg(ap, char *);
		_tprintf("<TEMPLATEGROUP name='%s'>\n",c);
		break;
	case TAG_TEMPLGRP_CLOSE:
  		_tprintf("</TEMPLATEGROUP>\n");
		break;	
	case TAG_VPNCONN_OPEN:
  		c=va_arg(ap, char *);
		_tprintf("<VPNCONNECTOR name='%s'>\n",c);
		break;
	case TAG_VPNCONN_CLOSE:
  		_tprintf("</VPNCONNECTOR>\n");
		break;
	case TAG_COND_OPEN:
  		c=va_arg(ap, char *);
		_tprintf("<CONDITION name='%s'>\n",c);
		break;
	case TAG_COND_CLOSE:
  		_tprintf("</CONDITION>\n");
		break;
	case TAG_GENERIC:
  		_tprintf("</GENERIC>\n");
		break;
        case TAG_DUMMY_BEGIN:
        	_tprintf("<NETXMS>\n");
		break;
        case TAG_DUMMY_END:
        	_tprintf("</NETXMS>\n");
		break;       
  }
 
  va_end(ap);
}
