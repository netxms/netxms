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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementLayout;
import org.netxms.ui.eclipse.tools.IntermediateSelectionProvider;
import org.netxms.ui.eclipse.widgets.DashboardComposite;

/**
 * Base class for all dashboard elements
 */
class ElementWidget extends DashboardComposite implements ControlListener
{
	protected DashboardElement element;
	protected IViewPart viewPart;
	
	private DashboardControl dbc;
	private DashboardElementLayout layout;
	private boolean editMode = false;
	private EditPaneWidget editPane = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	protected ElementWidget(DashboardControl parent, int style, DashboardElement element, IViewPart viewPart)
	{
		super(parent, style);
		dbc = parent;
		this.element = element;
		this.viewPart = viewPart;
		parseLayout(element.getLayout());
		addControlListener(this);
	}

	/**
	 * @param parent
	 * @param style
	 */
	protected ElementWidget(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, SWT.BORDER);
		dbc = parent;
		this.element = element;
		this.viewPart = viewPart;
		parseLayout(element.getLayout());
		addControlListener(this);
	}
	
	/**
	 * @param xml
	 */
	private void parseLayout(String xml)
	{
		try
		{
			layout = DashboardElementLayout.createFromXml(xml);
		}
		catch(Exception e)
		{
			e.printStackTrace();
			layout = new DashboardElementLayout();
		}
	}

	/**
	 * @return the layout
	 */
	public DashboardElementLayout getElementLayout()
	{
		return layout;
	}

	/**
	 * @return the editMode
	 */
	public boolean isEditMode()
	{
		return editMode;
	}

	/**
	 * @param editMode the editMode to set
	 */
	public void setEditMode(boolean editMode)
	{
		this.editMode = editMode;
		if (editMode)
		{			
			editPane = new EditPaneWidget(this, dbc, element);
			editPane.setLocation(0,  0);
			editPane.setSize(getSize());
			editPane.moveAbove(null);
		}
		else
		{
			if (editPane != null)
			{
				editPane.dispose();
				editPane = null;
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.ControlListener#controlMoved(org.eclipse.swt.events.ControlEvent)
	 */
	@Override
	public void controlMoved(ControlEvent e)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.ControlListener#controlResized(org.eclipse.swt.events.ControlEvent)
	 */
	@Override
	public void controlResized(ControlEvent e)
	{
		if (editPane != null)
		{
			editPane.setLocation(0,  0);
			editPane.setSize(getSize());
			editPane.moveAbove(null);
		}
	}
	
	/**
	 * Set delegate selection provider
	 * 
	 * @param delegate
	 */
	protected void setSelectionProviderDelegate(ISelectionProvider delegate)
	{
	   dbc.getSelectionProvider().setSelectionProviderDelegate(delegate);
	}
	
	/**
	 * Get intermediate selection provider
	 * 
	 * @return
	 */
	protected IntermediateSelectionProvider getSelectionProvider()
	{
	   return dbc.getSelectionProvider();
	}
}
