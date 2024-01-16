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
package org.netxms.nxmc.modules.networkmaps.widgets.helpers;

import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.FigureListener;
import org.eclipse.draw2d.IFigure;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.UnknownObject;
import org.netxms.nxmc.Registry;

/**
 * Abstract base class for all NetXMS object figures
 */
public abstract class ObjectFigure extends Figure
{
	protected static final Color SELECTION_COLOR = new Color(Display.getCurrent(), 132, 0, 200);
   protected static final Color SOLID_WHITE = new Color(Display.getCurrent(), 255, 255, 255);

	protected AbstractObject object;
	protected MapLabelProvider labelProvider;
	protected NetworkMapObject element;

	private boolean moved = false;

	/**
	 * Constructor
	 */
	public ObjectFigure(NetworkMapObject element, MapLabelProvider labelProvider)
	{
	   this.element = element;
		this.labelProvider = labelProvider;

		// set default font
      setFont(labelProvider.getLabelFont());

      NXCSession session = Registry.getSession();
      object = session.findObjectById(element.getObjectId(), true);
		if (object == null)
			object = new UnknownObject(element.getObjectId(), session);

		setFocusTraversable(true);

		addFigureListener(new FigureListener() {
			@Override
			public void figureMoved(IFigure source)
			{
				moved = true;
			}
		});
	}

   /**
    * @see org.eclipse.draw2d.Figure#getToolTip()
    */
   @Override
   public IFigure getToolTip()
   {
      IFigure tooltip = super.getToolTip();
      if ((tooltip != null) && (tooltip instanceof ObjectTooltip))
      {
         ((ObjectTooltip)tooltip).refresh();
      }
      return tooltip;
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
    *  Map element shown by this figure
    * 
    * @return element
    */
   public NetworkMapElement getMapElement()
   {
      return element;
   }
}
