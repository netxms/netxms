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
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.FigureListener;
import org.eclipse.draw2d.IFigure;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.UnknownObject;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Abstract base class for all NetXMS object figures
 */
public abstract class ObjectFigure extends Figure
{
	protected static final Color SELECTION_COLOR = new Color(Display.getCurrent(), 132, 0, 200);
	
	protected NetworkMapObject element;
	protected GenericObject object;
	protected MapLabelProvider labelProvider;
	
	private boolean moved = false;
	
	/**
	 * Constructor
	 */
	public ObjectFigure(NetworkMapObject element, MapLabelProvider labelProvider)
	{
		this.element = element;
		this.labelProvider = labelProvider;

		NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		object = session.findObjectById(element.getObjectId());
		if (object == null)
			object = new UnknownObject(element.getObjectId(), session);

		setFocusTraversable(true);
		setToolTip(new ObjectTooltip(object));
		
		addFigureListener(new FigureListener() {
			@Override
			public void figureMoved(IFigure source)
			{
				moved = true;
			}
		});
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#setToolTip(org.eclipse.draw2d.IFigure)
	 */
	@Override
	public void setToolTip(IFigure f)
	{
		// we set our own tooltip figure in object figure constructor
		// this check is to prevent overriding it by the viewer
		if ((f == null) || (f instanceof ObjectTooltip))
			super.setToolTip(f);
	}
	
	/**
	 * Check if associated map element is currently selected.
	 * 
	 * @return
	 */
	public boolean isElementSelected()
	{
		return labelProvider.isElementSelected(element);
	}
	
	/**
	 * Read figure's "moved" state. State reset to false after read.
	 * 
	 * @return
	 */
	public boolean readMovedState()
	{
		if (moved)
		{
			moved = false;
			return true;
		}
		return false;
	}

	/**
	 * Called by label provider to indicate object update
	 * @param object updated object
	 */
	protected void update()
	{
		NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		object = session.findObjectById(element.getObjectId());
		if (object == null)
			object = new UnknownObject(element.getObjectId(), session);
		setToolTip(new ObjectTooltip(object));
		onObjectUpdate();
		invalidateTree();
	}
	
	/**
	 * Object update handler.
	 */
	protected abstract void onObjectUpdate();
}
