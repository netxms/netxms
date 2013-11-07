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
package org.netxms.ui.eclipse.switchmanager.views.helpers;

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableFontProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.switchmanager.views.Dot1xStatusView;

/**
 * Label provider for 802.1x port summary
 */
public class Dot1xPortListLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider, ITableFontProvider
{
	private Color COLOR_UNKNOWN = new Color(Display.getCurrent(), 127, 127, 127);
	private Color COLOR_FAILURE = new Color(Display.getCurrent(), 255, 0, 0);
	private Color COLOR_POSSIBLE_FAILURE = new Color(Display.getCurrent(), 255, 128, 0);
	private Color COLOR_AUTH_AUTO = new Color(Display.getCurrent(), 0, 192, 0);
	private Color COLOR_AUTH_FORCED = new Color(Display.getCurrent(), 0, 0, 128);
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		Dot1xPortSummary port = (Dot1xPortSummary)element;
		switch(columnIndex)
		{
			case Dot1xStatusView.COLUMN_NODE:
				return port.getNodeName();
			case Dot1xStatusView.COLUMN_PORT:
				return port.getPortName();
			case Dot1xStatusView.COLUMN_INTERFACE:
				return port.getInterfaceName();
			case Dot1xStatusView.COLUMN_PAE_STATE:
				return port.getPaeStateAsText();
			case Dot1xStatusView.COLUMN_BACKEND_STATE:
				return port.getBackendStateAsText();
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
	 */
	@Override
	public Color getForeground(Object element, int columnIndex)
	{
		Dot1xPortSummary port = (Dot1xPortSummary)element;

		if ((port.getPaeState() == Interface.PAE_STATE_UNKNOWN) ||
		    (port.getPaeState() == Interface.PAE_STATE_INITIALIZE))
			return COLOR_UNKNOWN;

		if (port.getPaeState() == Interface.PAE_STATE_FORCE_AUTH)
			return COLOR_AUTH_FORCED;
		
		if (port.getPaeState() == Interface.PAE_STATE_AUTHENTICATED)
			return COLOR_AUTH_AUTO;
		
		if (port.getPaeState() == Interface.PAE_STATE_CONNECTING)
			return COLOR_POSSIBLE_FAILURE;
		
		if ((port.getPaeState() == Interface.PAE_STATE_FORCE_UNAUTH) ||
		    (port.getPaeState() == Interface.PAE_STATE_HELD) ||
		    (port.getBackendState() == Interface.BACKEND_STATE_FAIL) ||
		    (port.getBackendState() == Interface.BACKEND_STATE_TIMEOUT))
			return COLOR_FAILURE;
		
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
	 */
	@Override
	public Color getBackground(Object element, int columnIndex)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableFontProvider#getFont(java.lang.Object, int)
	 */
	@Override
	public Font getFont(Object element, int columnIndex)
	{
		Dot1xPortSummary port = (Dot1xPortSummary)element;

		if ((port.getPaeState() == Interface.PAE_STATE_FORCE_UNAUTH) ||
		    (port.getPaeState() == Interface.PAE_STATE_HELD) ||
		    (port.getBackendState() == Interface.BACKEND_STATE_FAIL) ||
		    (port.getBackendState() == Interface.BACKEND_STATE_TIMEOUT) ||
		    (port.getPaeState() == Interface.PAE_STATE_CONNECTING))
			return JFaceResources.getFontRegistry().getBold(JFaceResources.DEFAULT_FONT);
		
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		COLOR_AUTH_AUTO.dispose();
		COLOR_AUTH_FORCED.dispose();
		COLOR_FAILURE.dispose();
		COLOR_POSSIBLE_FAILURE.dispose();
		COLOR_UNKNOWN.dispose();
		super.dispose();
	}
}
