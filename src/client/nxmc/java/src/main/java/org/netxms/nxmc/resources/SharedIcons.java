/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
   public static ImageDescriptor CALENDAR;
	public static ImageDescriptor CHECKBOX_OFF;
	public static ImageDescriptor CHECKBOX_ON;
	public static ImageDescriptor CLEAR;
	public static ImageDescriptor CLEAR_LOG;
   public static ImageDescriptor CLONE;
	public static ImageDescriptor CLOSE;
	public static ImageDescriptor COLLAPSE;
	public static ImageDescriptor COLLAPSE_ALL;
   public static ImageDescriptor COMMENTS;
   public static ImageDescriptor CONTAINER;
	public static ImageDescriptor COPY;
   public static ImageDescriptor COPY_TO_CLIPBOARD;
	public static ImageDescriptor CSV;
   public static ImageDescriptor CUT;
   public static ImageDescriptor DATES;
   public static ImageDescriptor DELETE_OBJECT;
   public static ImageDescriptor DISABLE;
   public static ImageDescriptor DOWN;
   public static ImageDescriptor DOWNLOAD;
   public static ImageDescriptor EDIT;
	public static ImageDescriptor EMPTY;
	public static ImageDescriptor EXECUTE;
	public static ImageDescriptor EXPAND;
	public static ImageDescriptor EXPAND_ALL;
   public static ImageDescriptor EXPORT;
   public static ImageDescriptor FILTER;
	public static ImageDescriptor FIND;
   public static ImageDescriptor GROUP;
   public static ImageDescriptor HIDE;
   public static ImageDescriptor IMPORT;
   public static ImageDescriptor INFORMATION;
	public static ImageDescriptor IP_ADDRESS;
   public static ImageDescriptor JSON;
	public static ImageDescriptor NAV_BACKWARD;
	public static ImageDescriptor NAV_FORWARD;
	public static ImageDescriptor PASTE;
   public static ImageDescriptor PIN;
   public static ImageDescriptor POP_OUT;
   public static ImageDescriptor PREVIEW;
   public static ImageDescriptor PROPERTIES;
	public static ImageDescriptor REFRESH;
   public static ImageDescriptor RENAME;
	public static ImageDescriptor RESTART;
	public static ImageDescriptor SAVE;
   public static ImageDescriptor SAVE_AS;
   public static ImageDescriptor SAVE_AS_IMAGE;
   public static ImageDescriptor SEND;
   public static ImageDescriptor SHOW;
   public static ImageDescriptor SOUND;
	public static ImageDescriptor TERMINATE;
	public static ImageDescriptor UNKNOWN_OBJECT;
	public static ImageDescriptor UP;
   public static ImageDescriptor UPDATE;
   public static ImageDescriptor UPLOAD;
   public static ImageDescriptor URL;
   public static ImageDescriptor USER;
   public static ImageDescriptor VIEW_MENU;
   public static ImageDescriptor WAITING;
   public static ImageDescriptor XML;
	public static ImageDescriptor ZOOM_IN;
	public static ImageDescriptor ZOOM_OUT;

	public static Image IMG_ADD_OBJECT;
	public static Image IMG_ALARM;
   public static Image IMG_BROWSER;
   public static Image IMG_CALENDAR;
	public static Image IMG_CHECKBOX_OFF;
	public static Image IMG_CHECKBOX_ON;
	public static Image IMG_CLEAR;
	public static Image IMG_CLEAR_LOG;
   public static Image IMG_CLONE;
   public static Image IMG_CLOSE;
	public static Image IMG_COLLAPSE;
	public static Image IMG_COLLAPSE_ALL;
   public static Image IMG_COMMENTS;
   public static Image IMG_CONTAINER;
	public static Image IMG_COPY;
   public static Image IMG_COPY_TO_CLIPBOARD;
	public static Image IMG_CSV;
   public static Image IMG_CUT;
   public static Image IMG_DATES;
   public static Image IMG_DELETE_OBJECT;
   public static Image IMG_DISABLE;
   public static Image IMG_DOWN;
   public static Image IMG_DOWNLOAD;
	public static Image IMG_EDIT;
	public static Image IMG_EMPTY;
	public static Image IMG_EXECUTE;
	public static Image IMG_EXPAND;
	public static Image IMG_EXPAND_ALL;
   public static Image IMG_EXPORT;
	public static Image IMG_FILTER;
   public static Image IMG_FIND;
   public static Image IMG_GROUP;
   public static Image IMG_HIDE;
   public static Image IMG_IMPORT;
   public static Image IMG_INFORMATION;
	public static Image IMG_IP_ADDRESS;
   public static Image IMG_JSON;
	public static Image IMG_NAV_BACKWARD;
	public static Image IMG_NAV_FORWARD;
	public static Image IMG_PASTE;
   public static Image IMG_PIN;
   public static Image IMG_POP_OUT;
   public static Image IMG_PREVIEW;
   public static Image IMG_PROPERTIES;
	public static Image IMG_REFRESH;
   public static Image IMG_RENAME;
	public static Image IMG_RESTART;
	public static Image IMG_SAVE;
   public static Image IMG_SAVE_AS;
   public static Image IMG_SAVE_AS_IMAGE;
   public static Image IMG_SEND;
   public static Image IMG_SHOW;
   public static Image IMG_SOUND;
	public static Image IMG_TERMINATE;
	public static Image IMG_UNKNOWN_OBJECT;
	public static Image IMG_UP;
   public static Image IMG_UPDATE;
   public static Image IMG_UPLOAD;
   public static Image IMG_URL;
   public static Image IMG_USER;
   public static Image IMG_VIEW_MENU;
   public static Image IMG_WAITING;
   public static Image IMG_XML;
	public static Image IMG_ZOOM_IN;
	public static Image IMG_ZOOM_OUT;

	/**
    * Initialize static members. Intended to be called once by library ResourceManager.
    */
	public static void init()
	{
      ADD_OBJECT = ResourceManager.getImageDescriptor("icons/add_obj.png");
      ALARM = ResourceManager.getImageDescriptor("icons/alarm.png");
      BROWSER = ResourceManager.getImageDescriptor("icons/browser.png");
      CALENDAR = ResourceManager.getImageDescriptor("icons/calendar.png");
      CHECKBOX_OFF = ResourceManager.getImageDescriptor("icons/checkbox_off.png");
      CHECKBOX_ON = ResourceManager.getImageDescriptor("icons/checkbox_on.png");
      CLEAR = ResourceManager.getImageDescriptor("icons/clear.png");
      CLEAR_LOG = ResourceManager.getImageDescriptor("icons/clear_log.gif");
      CLONE = ResourceManager.getImageDescriptor("icons/clone.png");
      CLOSE = ResourceManager.getImageDescriptor("icons/close.png");
      COLLAPSE = ResourceManager.getImageDescriptor("icons/collapse.png");
      COLLAPSE_ALL = ResourceManager.getImageDescriptor("icons/collapseall.png");
      COMMENTS = ResourceManager.getImageDescriptor("icons/comments.png");
      CONTAINER = ResourceManager.getImageDescriptor("icons/container.png");
      COPY = ResourceManager.getImageDescriptor("icons/copy.gif");
      COPY_TO_CLIPBOARD = ResourceManager.getImageDescriptor("icons/copy-to-clipboard.png");
      CSV = ResourceManager.getImageDescriptor("icons/csv.png");
      CUT = ResourceManager.getImageDescriptor("icons/cut.gif");
      DATES = ResourceManager.getImageDescriptor("icons/dates.png");
      DELETE_OBJECT = ResourceManager.getImageDescriptor("icons/delete_obj.png");
      DISABLE = ResourceManager.getImageDescriptor("icons/disable.png");
      DOWN = ResourceManager.getImageDescriptor("icons/down.png");
      DOWNLOAD = ResourceManager.getImageDescriptor("icons/download.png");
      EDIT = ResourceManager.getImageDescriptor("icons/edit.png");
      EMPTY = ResourceManager.getImageDescriptor("icons/empty.png");
      EXECUTE = ResourceManager.getImageDescriptor("icons/execute.png");
      EXPAND = ResourceManager.getImageDescriptor("icons/expand.png");
      EXPAND_ALL = ResourceManager.getImageDescriptor("icons/expandall.png");
      EXPORT = ResourceManager.getImageDescriptor("icons/export.png");
      FILTER = ResourceManager.getImageDescriptor("icons/filter.png");
      FIND = ResourceManager.getImageDescriptor("icons/find.png");
      GROUP = ResourceManager.getImageDescriptor("icons/group.png");
      HIDE = ResourceManager.getImageDescriptor("icons/hide.png");
      IMPORT = ResourceManager.getImageDescriptor("icons/import.png");
      INFORMATION = ResourceManager.getImageDescriptor("icons/information.png");
      IP_ADDRESS = ResourceManager.getImageDescriptor("icons/ipaddr.png");
      JSON = ResourceManager.getImageDescriptor("icons/json.png");
      NAV_BACKWARD = ResourceManager.getImageDescriptor("icons/backward_nav.png");
      NAV_FORWARD = ResourceManager.getImageDescriptor("icons/forward_nav.png");
      PASTE = ResourceManager.getImageDescriptor("icons/paste.gif");
      PIN = ResourceManager.getImageDescriptor("icons/pin.png");
      POP_OUT = ResourceManager.getImageDescriptor("icons/pop-out.png");
      PREVIEW = ResourceManager.getImageDescriptor("icons/preview.png");
      PROPERTIES = ResourceManager.getImageDescriptor("icons/properties.png");
      REFRESH = ResourceManager.getImageDescriptor("icons/refresh.png");
      RENAME = ResourceManager.getImageDescriptor("icons/rename.png");
      RESTART = ResourceManager.getImageDescriptor("icons/restart.gif");
      SAVE = ResourceManager.getImageDescriptor("icons/save.png");
      SAVE_AS = ResourceManager.getImageDescriptor("icons/save-as.png");
      SAVE_AS_IMAGE = ResourceManager.getImageDescriptor("icons/image_obj.png");
      SEND = ResourceManager.getImageDescriptor("icons/send.png");
      SHOW = ResourceManager.getImageDescriptor("icons/show.png");
      SOUND = ResourceManager.getImageDescriptor("icons/sound.png");
      TERMINATE = ResourceManager.getImageDescriptor("icons/terminate.png");
      UNKNOWN_OBJECT = ResourceManager.getImageDescriptor("icons/unknown_obj.gif");
      UP = ResourceManager.getImageDescriptor("icons/up.png");
      UPDATE = ResourceManager.getImageDescriptor("icons/update.png");
      UPLOAD = ResourceManager.getImageDescriptor("icons/upload.png");
      URL = ResourceManager.getImageDescriptor("icons/url.png");
      USER = ResourceManager.getImageDescriptor("icons/user.png");
      VIEW_MENU = ResourceManager.getImageDescriptor("icons/view_menu.png");
      WAITING = ResourceManager.getImageDescriptor("icons/waiting.png");
      XML = ResourceManager.getImageDescriptor("icons/xml.gif");
      ZOOM_IN = ResourceManager.getImageDescriptor("icons/zoom-in.png");
      ZOOM_OUT = ResourceManager.getImageDescriptor("icons/zoom-out.png");

		IMG_ADD_OBJECT = ADD_OBJECT.createImage();
		IMG_ALARM = ALARM.createImage();
      IMG_BROWSER = BROWSER.createImage();
      IMG_CALENDAR = CALENDAR.createImage();
		IMG_CHECKBOX_OFF = CHECKBOX_OFF.createImage();
		IMG_CHECKBOX_ON = CHECKBOX_ON.createImage();
		IMG_CLEAR = CLEAR.createImage();
		IMG_CLEAR_LOG = CLEAR_LOG.createImage();
      IMG_CLONE = CLONE.createImage();
		IMG_CLOSE = CLOSE.createImage();
		IMG_COLLAPSE = COLLAPSE.createImage();
		IMG_COLLAPSE_ALL = COLLAPSE_ALL.createImage();
      IMG_COMMENTS = COMMENTS.createImage();
		IMG_CONTAINER = CONTAINER.createImage();
		IMG_COPY = COPY.createImage();
      IMG_COPY_TO_CLIPBOARD = COPY_TO_CLIPBOARD.createImage();
		IMG_CSV = CSV.createImage();
      IMG_CUT = CUT.createImage();
      IMG_DATES = DATES.createImage();
		IMG_DELETE_OBJECT = DELETE_OBJECT.createImage();
      IMG_DISABLE = DISABLE.createImage();
		IMG_DOWN = DOWN.createImage();
      IMG_DOWNLOAD = DOWNLOAD.createImage();
		IMG_EDIT = EDIT.createImage();
		IMG_EMPTY = EMPTY.createImage();
		IMG_EXECUTE = EXECUTE.createImage();
		IMG_EXPAND = EXPAND.createImage();
		IMG_EXPAND_ALL = EXPAND_ALL.createImage();
      IMG_EXPORT = EXPORT.createImage();
		IMG_FILTER = FILTER.createImage();
      IMG_FIND = FIND.createImage();
      IMG_GROUP = GROUP.createImage();
      IMG_HIDE = HIDE.createImage();
      IMG_IMPORT = IMPORT.createImage();
      IMG_INFORMATION = INFORMATION.createImage();
		IMG_IP_ADDRESS = IP_ADDRESS.createImage();
      IMG_JSON = JSON.createImage();
		IMG_NAV_BACKWARD = NAV_BACKWARD.createImage();
		IMG_NAV_FORWARD = NAV_FORWARD.createImage();
		IMG_PASTE = PASTE.createImage();
      IMG_PIN = PIN.createImage();
      IMG_POP_OUT = POP_OUT.createImage();
      IMG_PREVIEW = PREVIEW.createImage();
      IMG_PROPERTIES = PROPERTIES.createImage();
		IMG_REFRESH = REFRESH.createImage();
      IMG_RENAME = RENAME.createImage();
		IMG_RESTART = RESTART.createImage();
		IMG_SAVE = SAVE.createImage();
      IMG_SAVE_AS = SAVE_AS.createImage();
      IMG_SAVE_AS_IMAGE = SAVE_AS_IMAGE.createImage();
      IMG_SEND = SEND.createImage();
      IMG_SHOW = SHOW.createImage();
      IMG_SOUND = SOUND.createImage();
		IMG_TERMINATE = TERMINATE.createImage();
		IMG_UNKNOWN_OBJECT = UNKNOWN_OBJECT.createImage();
		IMG_UP = UP.createImage();
      IMG_UPDATE = UPDATE.createImage();
      IMG_UPLOAD = UPLOAD.createImage();
      IMG_URL = URL.createImage();
      IMG_USER = USER.createImage();
      IMG_VIEW_MENU = VIEW_MENU.createImage();
      IMG_WAITING = WAITING.createImage();
      IMG_XML = XML.createImage();
		IMG_ZOOM_IN = ZOOM_IN.createImage();
		IMG_ZOOM_OUT = ZOOM_OUT.createImage();
	}
}
