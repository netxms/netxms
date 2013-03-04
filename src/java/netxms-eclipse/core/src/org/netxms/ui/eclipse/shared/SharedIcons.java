/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.shared;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.Image;
import org.netxms.ui.eclipse.console.Activator;

/**
 * Shared console icons
 */
public class SharedIcons
{
	public static ImageDescriptor ADD_OBJECT;
	public static ImageDescriptor ALARM;
	public static ImageDescriptor CHECKBOX_OFF;
	public static ImageDescriptor CHECKBOX_ON;
	public static ImageDescriptor CLEAR;
	public static ImageDescriptor CLEAR_LOG;
	public static ImageDescriptor CLOSE;
	public static ImageDescriptor COLLAPSE;
	public static ImageDescriptor COLLAPSE_ALL;
	public static ImageDescriptor COPY;
	public static ImageDescriptor CUT;
	public static ImageDescriptor DELETE_OBJECT;
	public static ImageDescriptor DOWN;
	public static ImageDescriptor EDIT;
	public static ImageDescriptor EMPTY;
	public static ImageDescriptor EXECUTE;
	public static ImageDescriptor EXPAND;
	public static ImageDescriptor EXPAND_ALL;
	public static ImageDescriptor FIND;
	public static ImageDescriptor IP_ADDRESS;
	public static ImageDescriptor NAV_BACKWARD;
	public static ImageDescriptor NAV_FORWARD;
	public static ImageDescriptor PASTE;
	public static ImageDescriptor REFRESH;
	public static ImageDescriptor RESTART;
	public static ImageDescriptor SAVE;
	public static ImageDescriptor TERMINATE;
	public static ImageDescriptor UNKNOWN_OBJECT;
	public static ImageDescriptor UP;
	public static ImageDescriptor XML;
	public static ImageDescriptor ZOOM_IN;
	public static ImageDescriptor ZOOM_OUT;
	
	public static Image IMG_ADD_OBJECT;
	public static Image IMG_ALARM;
	public static Image IMG_CHECKBOX_OFF;
	public static Image IMG_CHECKBOX_ON;
	public static Image IMG_CLEAR;
	public static Image IMG_CLEAR_LOG;
	public static Image IMG_CLOSE;
	public static Image IMG_COLLAPSE;
	public static Image IMG_COLLAPSE_ALL;
	public static Image IMG_COPY;
	public static Image IMG_CUT;
	public static Image IMG_DELETE_OBJECT;
	public static Image IMG_DOWN;
	public static Image IMG_EDIT;
	public static Image IMG_EMPTY;
	public static Image IMG_EXECUTE;
	public static Image IMG_EXPAND;
	public static Image IMG_EXPAND_ALL;
	public static Image IMG_FIND;
	public static Image IMG_IP_ADDRESS;
	public static Image IMG_NAV_BACKWARD;
	public static Image IMG_NAV_FORWARD;
	public static Image IMG_PASTE;
	public static Image IMG_REFRESH;
	public static Image IMG_RESTART;
	public static Image IMG_SAVE;
	public static Image IMG_TERMINATE;
	public static Image IMG_UNKNOWN_OBJECT;
	public static Image IMG_UP;
	public static Image IMG_XML;
	public static Image IMG_ZOOM_IN;
	public static Image IMG_ZOOM_OUT;
	
	/**
	 * Initialize static members. Intended to be called once by library activator.
	 */
	public static void init()
	{
		ALARM = Activator.getImageDescriptor("icons/alarm.png"); //$NON-NLS-1$
		ADD_OBJECT = Activator.getImageDescriptor("icons/add_obj.gif"); //$NON-NLS-1$
		CHECKBOX_OFF = Activator.getImageDescriptor("icons/checkbox_off.png"); //$NON-NLS-1$
		CHECKBOX_ON = Activator.getImageDescriptor("icons/checkbox_on.png"); //$NON-NLS-1$
		CLEAR = Activator.getImageDescriptor("icons/clear.gif"); //$NON-NLS-1$
		CLEAR_LOG = Activator.getImageDescriptor("icons/clear_log.gif"); //$NON-NLS-1$
		CLOSE = Activator.getImageDescriptor("icons/close.gif"); //$NON-NLS-1$
		COLLAPSE = Activator.getImageDescriptor("icons/collapse.png"); //$NON-NLS-1$
		COLLAPSE_ALL = Activator.getImageDescriptor("icons/collapseall.png"); //$NON-NLS-1$
		COPY = Activator.getImageDescriptor("icons/copy.gif"); //$NON-NLS-1$
		CUT = Activator.getImageDescriptor("icons/cut.gif"); //$NON-NLS-1$
		DELETE_OBJECT = Activator.getImageDescriptor("icons/delete_obj.gif"); //$NON-NLS-1$
		DOWN = Activator.getImageDescriptor("icons/down.png"); //$NON-NLS-1$
		EDIT = Activator.getImageDescriptor("icons/edit.png"); //$NON-NLS-1$
		EMPTY = Activator.getImageDescriptor("icons/empty.png"); //$NON-NLS-1$
		EXECUTE = Activator.getImageDescriptor("icons/execute.gif"); //$NON-NLS-1$
		EXPAND = Activator.getImageDescriptor("icons/expand.png"); //$NON-NLS-1$
		EXPAND_ALL = Activator.getImageDescriptor("icons/expandall.png"); //$NON-NLS-1$
		FIND = Activator.getImageDescriptor("icons/find.gif"); //$NON-NLS-1$
		IP_ADDRESS = Activator.getImageDescriptor("icons/ipaddr.png"); //$NON-NLS-1$
		NAV_BACKWARD = Activator.getImageDescriptor("icons/nav_backward.gif"); //$NON-NLS-1$
		NAV_FORWARD = Activator.getImageDescriptor("icons/nav_forward.gif"); //$NON-NLS-1$
		PASTE = Activator.getImageDescriptor("icons/paste.gif"); //$NON-NLS-1$
		REFRESH = Activator.getImageDescriptor("icons/refresh.gif"); //$NON-NLS-1$
		RESTART = Activator.getImageDescriptor("icons/restart.gif"); //$NON-NLS-1$
		SAVE = Activator.getImageDescriptor("icons/save.gif"); //$NON-NLS-1$
		TERMINATE = Activator.getImageDescriptor("icons/terminate.gif"); //$NON-NLS-1$
		UNKNOWN_OBJECT = Activator.getImageDescriptor("icons/unknown_obj.gif"); //$NON-NLS-1$
		UP = Activator.getImageDescriptor("icons/up.png"); //$NON-NLS-1$
		XML = Activator.getImageDescriptor("icons/xml.gif"); //$NON-NLS-1$
		ZOOM_IN = Activator.getImageDescriptor("icons/zoom_in.png"); //$NON-NLS-1$
		ZOOM_OUT = Activator.getImageDescriptor("icons/zoom_out.png"); //$NON-NLS-1$

		IMG_ADD_OBJECT = ADD_OBJECT.createImage();
		IMG_ALARM = ALARM.createImage();
		IMG_CHECKBOX_OFF = CHECKBOX_OFF.createImage();
		IMG_CHECKBOX_ON = CHECKBOX_ON.createImage();
		IMG_CLEAR = CLEAR.createImage();
		IMG_CLEAR_LOG = CLEAR_LOG.createImage();
		IMG_CLOSE = CLOSE.createImage();
		IMG_COLLAPSE = COLLAPSE.createImage();
		IMG_COLLAPSE_ALL = COLLAPSE_ALL.createImage();
		IMG_COPY = COPY.createImage();
		IMG_CUT = CUT.createImage();
		IMG_DELETE_OBJECT = DELETE_OBJECT.createImage();
		IMG_DOWN = DOWN.createImage();
		IMG_EDIT = EDIT.createImage();
		IMG_EMPTY = EMPTY.createImage();
		IMG_EXECUTE = EXECUTE.createImage();
		IMG_EXPAND = EXPAND.createImage();
		IMG_EXPAND_ALL = EXPAND_ALL.createImage();
		IMG_FIND = FIND.createImage();
		IMG_IP_ADDRESS = IP_ADDRESS.createImage();
		IMG_NAV_BACKWARD = NAV_BACKWARD.createImage();
		IMG_NAV_FORWARD = NAV_FORWARD.createImage();
		IMG_PASTE = PASTE.createImage();
		IMG_REFRESH = REFRESH.createImage();
		IMG_RESTART = RESTART.createImage();
		IMG_SAVE = SAVE.createImage();
		IMG_TERMINATE = TERMINATE.createImage();
		IMG_UNKNOWN_OBJECT = UNKNOWN_OBJECT.createImage();
		IMG_UP = UP.createImage();
		IMG_XML = XML.createImage();
		IMG_ZOOM_IN = ZOOM_IN.createImage();
		IMG_ZOOM_OUT = ZOOM_OUT.createImage();
	}
}
