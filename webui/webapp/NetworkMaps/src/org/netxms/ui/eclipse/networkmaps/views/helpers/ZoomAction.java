/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.jface.action.Action;
import org.eclipse.zest.core.widgets.zooming.ZoomListener;
import org.eclipse.zest.core.widgets.zooming.ZoomManager;

/**
 * Zoom action
 *
 */
public class ZoomAction extends Action implements ZoomListener
{
	private static final long serialVersionUID = 1L;

	private double zoomLevel;
	private ZoomManager zoomManager;
	
	/**
	 * Create zoom action for given zoom level
	 * 
	 * @param zoomLevel zoom level
	 */
	public ZoomAction(double zoomLevel, ZoomManager zoomManager)
	{
		super(Integer.toString((int)(zoomLevel * 100)) + "%", Action.AS_RADIO_BUTTON);
		this.zoomLevel = zoomLevel;
		this.zoomManager = zoomManager;
		zoomManager.addZoomListener(this);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.zest.core.viewers.internal.ZoomListener#zoomChanged(double)
	 */
	@Override
	public void zoomChanged(double zoom)
	{
		setChecked(zoom == zoomLevel);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.action.Action#run()
	 */
	@Override
	public void run()
	{
		zoomManager.setZoom(zoomLevel);
	}
}
