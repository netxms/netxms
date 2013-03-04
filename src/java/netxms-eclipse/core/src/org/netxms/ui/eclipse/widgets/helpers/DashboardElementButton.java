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
package org.netxms.ui.eclipse.widgets.helpers;

import org.eclipse.jface.action.Action;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Control;

/**
 * Dashboard element's button
 *
 */
public class DashboardElementButton
{
	private String name;
	private Image image;
	private Action action;
	private Control control;

	/**
	 * @param name
	 * @param image
	 * @param action
	 */
	public DashboardElementButton(String name, Image image, Action action)
	{
		this.name = name;
		this.image = image;
		this.action = action;
		this.control = null;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the image
	 */
	public Image getImage()
	{
		return image;
	}

	/**
	 * @return the action
	 */
	public Action getAction()
	{
		return action;
	}

	/**
	 * Get associated control.<p>
	 * 
	 * <b>IMPORTANT</b>: This method is not part of public API. It is marked public 
	 * only so that it can be shared within NetXMS library packages.
	 * 
	 * @return the control
	 */
	public Control getControl()
	{
		return control;
	}

	/**
	 * Set associated control.<p>
	 * 
	 * <b>IMPORTANT</b>: This method is not part of public API. It is marked public 
	 * only so that it can be shared within NetXMS library packages.
	 * 
	 * @param control the control to set
	 */
	public void setControl(Control control)
	{
		this.control = control;
	}
}
