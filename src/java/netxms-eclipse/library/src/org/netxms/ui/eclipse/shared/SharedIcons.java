/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.netxms.ui.eclipse.library.Activator;

/**
 * Shared console icons
 *
 */
public class SharedIcons
{
	public static ImageDescriptor ADD_OBJECT;
	public static ImageDescriptor ALARM;
	public static ImageDescriptor CHECKBOX_OFF;
	public static ImageDescriptor CHECKBOX_ON;
	public static ImageDescriptor CLEAR_LOG;
	public static ImageDescriptor COLLAPSE;
	public static ImageDescriptor COLLAPSE_ALL;
	public static ImageDescriptor COPY;
	public static ImageDescriptor CUT;
	public static ImageDescriptor DELETE_OBJECT;
	public static ImageDescriptor EDIT;
	public static ImageDescriptor EXPAND;
	public static ImageDescriptor EXPAND_ALL;
	public static ImageDescriptor FIND;
	public static ImageDescriptor PASTE;
	public static ImageDescriptor REFRESH;
	public static ImageDescriptor SAVE;
	public static ImageDescriptor ZOOM_IN;
	public static ImageDescriptor ZOOM_OUT;
	
	/**
	 * Initialize static members. Intended to be called once by library activator.
	 */
	public static void init()
	{
		ALARM = Activator.getImageDescriptor("icons/alarm.png");
		ADD_OBJECT = Activator.getImageDescriptor("icons/add_obj.gif");
		CHECKBOX_OFF = Activator.getImageDescriptor("icons/checkbox_off.png");
		CHECKBOX_ON = Activator.getImageDescriptor("icons/checkbox_on.png");
		CLEAR_LOG = Activator.getImageDescriptor("icons/clear_log.gif");
		COLLAPSE = Activator.getImageDescriptor("icons/collapse.png");
		COLLAPSE_ALL = Activator.getImageDescriptor("icons/collapseall.png");
		COPY = Activator.getImageDescriptor("icons/copy.gif");
		CUT = Activator.getImageDescriptor("icons/cut.gif");
		DELETE_OBJECT = Activator.getImageDescriptor("icons/delete_obj.gif");
		EDIT = Activator.getImageDescriptor("icons/edit.png");
		EXPAND = Activator.getImageDescriptor("icons/expand.png");
		EXPAND_ALL = Activator.getImageDescriptor("icons/expandall.png");
		FIND = Activator.getImageDescriptor("icons/find.gif");
		PASTE = Activator.getImageDescriptor("icons/paste.gif");
		REFRESH = Activator.getImageDescriptor("icons/refresh.gif");
		SAVE = Activator.getImageDescriptor("icons/save.gif");
		ZOOM_IN = Activator.getImageDescriptor("icons/zoom_in.png");
		ZOOM_OUT = Activator.getImageDescriptor("icons/zoom_out.png");
	}
}
