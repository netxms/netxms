/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.nxmc.resources;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.Image;

/**
 * Shared console icons
 */
public class SharedIcons
{
	public static ImageDescriptor ADD_OBJECT;
	public static ImageDescriptor ALARM;
   public static ImageDescriptor BROWSER;
	public static ImageDescriptor CHECKBOX_OFF;
	public static ImageDescriptor CHECKBOX_ON;
	public static ImageDescriptor CLEAR;
	public static ImageDescriptor CLEAR_LOG;
	public static ImageDescriptor CLOSE;
	public static ImageDescriptor COLLAPSE;
	public static ImageDescriptor COLLAPSE_ALL;
   public static ImageDescriptor COMMENTS;
   public static ImageDescriptor CONTAINER;
	public static ImageDescriptor COPY;
	public static ImageDescriptor CSV;
   public static ImageDescriptor CUT;
	public static ImageDescriptor DELETE_OBJECT;
	public static ImageDescriptor DOWN;
	public static ImageDescriptor EDIT;
	public static ImageDescriptor EMPTY;
	public static ImageDescriptor EXECUTE;
	public static ImageDescriptor EXPAND;
	public static ImageDescriptor EXPAND_ALL;
   public static ImageDescriptor FILTER;
	public static ImageDescriptor FIND;
   public static ImageDescriptor GROUP;
   public static ImageDescriptor INFORMATION;
	public static ImageDescriptor IP_ADDRESS;
	public static ImageDescriptor NAV_BACKWARD;
	public static ImageDescriptor NAV_FORWARD;
	public static ImageDescriptor PASTE;
   public static ImageDescriptor PIN;
   public static ImageDescriptor POP_OUT;
	public static ImageDescriptor REFRESH;
	public static ImageDescriptor RESTART;
	public static ImageDescriptor SAVE;
   public static ImageDescriptor SAVE_AS;
   public static ImageDescriptor SAVE_AS_IMAGE;
	public static ImageDescriptor TERMINATE;
	public static ImageDescriptor UNKNOWN_OBJECT;
	public static ImageDescriptor UP;
   public static ImageDescriptor USER;
	public static ImageDescriptor XML;
	public static ImageDescriptor ZOOM_IN;
	public static ImageDescriptor ZOOM_OUT;
	
	public static Image IMG_ADD_OBJECT;
	public static Image IMG_ALARM;
   public static Image IMG_BROWSER;
	public static Image IMG_CHECKBOX_OFF;
	public static Image IMG_CHECKBOX_ON;
	public static Image IMG_CLEAR;
	public static Image IMG_CLEAR_LOG;
	public static Image IMG_CLOSE;
	public static Image IMG_COLLAPSE;
	public static Image IMG_COLLAPSE_ALL;
   public static Image IMG_COMMENTS;
   public static Image IMG_CONTAINER;
	public static Image IMG_COPY;
	public static Image IMG_CSV;
   public static Image IMG_CUT;
	public static Image IMG_DELETE_OBJECT;
	public static Image IMG_DOWN;
	public static Image IMG_EDIT;
	public static Image IMG_EMPTY;
	public static Image IMG_EXECUTE;
	public static Image IMG_EXPAND;
	public static Image IMG_EXPAND_ALL;
	public static Image IMG_FILTER;
   public static Image IMG_FIND;
   public static Image IMG_GROUP;
   public static Image IMG_INFORMATION;
	public static Image IMG_IP_ADDRESS;
	public static Image IMG_NAV_BACKWARD;
	public static Image IMG_NAV_FORWARD;
	public static Image IMG_PASTE;
   public static Image IMG_PIN;
   public static Image IMG_POP_OUT;
	public static Image IMG_REFRESH;
	public static Image IMG_RESTART;
	public static Image IMG_SAVE;
   public static Image IMG_SAVE_AS;
   public static Image IMG_SAVE_AS_IMAGE;
	public static Image IMG_TERMINATE;
	public static Image IMG_UNKNOWN_OBJECT;
	public static Image IMG_UP;
   public static Image IMG_USER;
	public static Image IMG_XML;
	public static Image IMG_ZOOM_IN;
	public static Image IMG_ZOOM_OUT;
	
	/**
    * Initialize static members. Intended to be called once by library ResourceManager.
    */
	public static void init()
	{
      ADD_OBJECT = ResourceManager.getImageDescriptor("icons/add_obj.gif"); //$NON-NLS-1$
      ALARM = ResourceManager.getImageDescriptor("icons/alarm.png"); //$NON-NLS-1$
      BROWSER = ResourceManager.getImageDescriptor("icons/browser.png"); //$NON-NLS-1$
      CHECKBOX_OFF = ResourceManager.getImageDescriptor("icons/checkbox_off.png"); //$NON-NLS-1$
      CHECKBOX_ON = ResourceManager.getImageDescriptor("icons/checkbox_on.png"); //$NON-NLS-1$
      CLEAR = ResourceManager.getImageDescriptor("icons/clear.gif"); //$NON-NLS-1$
      CLEAR_LOG = ResourceManager.getImageDescriptor("icons/clear_log.gif"); //$NON-NLS-1$
      CLOSE = ResourceManager.getImageDescriptor("icons/close.gif"); //$NON-NLS-1$
      COLLAPSE = ResourceManager.getImageDescriptor("icons/collapse.png"); //$NON-NLS-1$
      COLLAPSE_ALL = ResourceManager.getImageDescriptor("icons/collapseall.png"); //$NON-NLS-1$
      COMMENTS = ResourceManager.getImageDescriptor("icons/comments.png");
      CONTAINER = ResourceManager.getImageDescriptor("icons/container.png"); //$NON-NLS-1$
      COPY = ResourceManager.getImageDescriptor("icons/copy.gif"); //$NON-NLS-1$
      CSV = ResourceManager.getImageDescriptor("icons/csv.png"); //$NON-NLS-1$
      CUT = ResourceManager.getImageDescriptor("icons/cut.gif"); //$NON-NLS-1$
      DELETE_OBJECT = ResourceManager.getImageDescriptor("icons/delete_obj.gif"); //$NON-NLS-1$
      DOWN = ResourceManager.getImageDescriptor("icons/down.png"); //$NON-NLS-1$
      EDIT = ResourceManager.getImageDescriptor("icons/edit.png"); //$NON-NLS-1$
      EMPTY = ResourceManager.getImageDescriptor("icons/empty.png"); //$NON-NLS-1$
      EXECUTE = ResourceManager.getImageDescriptor("icons/execute.gif"); //$NON-NLS-1$
      EXPAND = ResourceManager.getImageDescriptor("icons/expand.png"); //$NON-NLS-1$
      EXPAND_ALL = ResourceManager.getImageDescriptor("icons/expandall.png"); //$NON-NLS-1$
      FILTER = ResourceManager.getImageDescriptor("icons/filter.gif"); //$NON-NLS-1$
      FIND = ResourceManager.getImageDescriptor("icons/find.gif"); //$NON-NLS-1$
      GROUP = ResourceManager.getImageDescriptor("icons/group.png"); //$NON-NLS-1$
      INFORMATION = ResourceManager.getImageDescriptor("icons/information.png"); //$NON-NLS-1$
      IP_ADDRESS = ResourceManager.getImageDescriptor("icons/ipaddr.png"); //$NON-NLS-1$
      NAV_BACKWARD = ResourceManager.getImageDescriptor("icons/nav_backward.gif"); //$NON-NLS-1$
      NAV_FORWARD = ResourceManager.getImageDescriptor("icons/nav_forward.gif"); //$NON-NLS-1$
      PASTE = ResourceManager.getImageDescriptor("icons/paste.gif"); //$NON-NLS-1$
      PIN = ResourceManager.getImageDescriptor("icons/pin.png"); //$NON-NLS-1$
      POP_OUT = ResourceManager.getImageDescriptor("icons/pop-out.png"); //$NON-NLS-1$
      REFRESH = ResourceManager.getImageDescriptor("icons/refresh.gif"); //$NON-NLS-1$
      RESTART = ResourceManager.getImageDescriptor("icons/restart.gif"); //$NON-NLS-1$
      SAVE = ResourceManager.getImageDescriptor("icons/save.gif"); //$NON-NLS-1$
      SAVE_AS = ResourceManager.getImageDescriptor("icons/saveas.gif"); //$NON-NLS-1$
      SAVE_AS_IMAGE = ResourceManager.getImageDescriptor("icons/image_obj.png"); //$NON-NLS-1$
      TERMINATE = ResourceManager.getImageDescriptor("icons/terminate.gif"); //$NON-NLS-1$
      UNKNOWN_OBJECT = ResourceManager.getImageDescriptor("icons/unknown_obj.gif"); //$NON-NLS-1$
      UP = ResourceManager.getImageDescriptor("icons/up.png"); //$NON-NLS-1$
      USER = ResourceManager.getImageDescriptor("icons/user.png"); //$NON-NLS-1$
      XML = ResourceManager.getImageDescriptor("icons/xml.gif"); //$NON-NLS-1$
      ZOOM_IN = ResourceManager.getImageDescriptor("icons/zoom_in.png"); //$NON-NLS-1$
      ZOOM_OUT = ResourceManager.getImageDescriptor("icons/zoom_out.png"); //$NON-NLS-1$

		IMG_ADD_OBJECT = ADD_OBJECT.createImage();
		IMG_ALARM = ALARM.createImage();
      IMG_BROWSER = BROWSER.createImage();
		IMG_CHECKBOX_OFF = CHECKBOX_OFF.createImage();
		IMG_CHECKBOX_ON = CHECKBOX_ON.createImage();
		IMG_CLEAR = CLEAR.createImage();
		IMG_CLEAR_LOG = CLEAR_LOG.createImage();
		IMG_CLOSE = CLOSE.createImage();
		IMG_COLLAPSE = COLLAPSE.createImage();
		IMG_COLLAPSE_ALL = COLLAPSE_ALL.createImage();
      IMG_COMMENTS = COMMENTS.createImage();
		IMG_CONTAINER = CONTAINER.createImage();
		IMG_COPY = COPY.createImage();
		IMG_CSV = CSV.createImage();
      IMG_CUT = CUT.createImage();
		IMG_DELETE_OBJECT = DELETE_OBJECT.createImage();
		IMG_DOWN = DOWN.createImage();
		IMG_EDIT = EDIT.createImage();
		IMG_EMPTY = EMPTY.createImage();
		IMG_EXECUTE = EXECUTE.createImage();
		IMG_EXPAND = EXPAND.createImage();
		IMG_EXPAND_ALL = EXPAND_ALL.createImage();
		IMG_FILTER = FILTER.createImage();
      IMG_FIND = FIND.createImage();
      IMG_GROUP = GROUP.createImage();
      IMG_INFORMATION = INFORMATION.createImage();
		IMG_IP_ADDRESS = IP_ADDRESS.createImage();
		IMG_NAV_BACKWARD = NAV_BACKWARD.createImage();
		IMG_NAV_FORWARD = NAV_FORWARD.createImage();
		IMG_PASTE = PASTE.createImage();
      IMG_PIN = PIN.createImage();
      IMG_POP_OUT = POP_OUT.createImage();
		IMG_REFRESH = REFRESH.createImage();
		IMG_RESTART = RESTART.createImage();
		IMG_SAVE = SAVE.createImage();
      IMG_SAVE_AS = SAVE_AS.createImage();
      IMG_SAVE_AS_IMAGE = SAVE_AS_IMAGE.createImage();
		IMG_TERMINATE = TERMINATE.createImage();
		IMG_UNKNOWN_OBJECT = UNKNOWN_OBJECT.createImage();
		IMG_UP = UP.createImage();
      IMG_USER = USER.createImage();
		IMG_XML = XML.createImage();
		IMG_ZOOM_IN = ZOOM_IN.createImage();
		IMG_ZOOM_OUT = ZOOM_OUT.createImage();
	}
}
