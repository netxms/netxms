/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectbrowser.widgets;

import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.client.constants.Severity;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectTreeViewer;
import org.netxms.ui.eclipse.shared.SharedColors;

/**
 * Object status indicator
 */
public class ObjectStatusIndicator extends Canvas implements PaintListener
{
	private ObjectTreeViewer objectTree = null;
	private boolean showIcons = false;
	private boolean showNormal = false;
	private boolean showUnmanaged = false;
	private boolean showUnknown = false;
	private boolean showDisabled = false;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectStatusIndicator(Composite parent, int style)
	{
		super(parent, style | SWT.DOUBLE_BUFFERED);
		addPaintListener(this);
		setBackground(SharedColors.WHITE);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		return new Point(20, 10);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		if (objectTree == null)
			return;
		
		TreeItem item = objectTree.getTree().getTopItem();
		if (item == null)
			return;
		
		final GC gc = e.gc;
		gc.setAntialias(SWT.ON);
		int y = 0;
		final int limit = objectTree.getTree().getClientArea().height;
		final int width = getClientArea().width;
		final int height = objectTree.getTree().getItemHeight();

		ViewerRow row = objectTree.getTreeViewerRow(item);
		while((row != null) && (y < limit))
		{
			GenericObject object = (GenericObject)row.getItem().getData();
			drawObject(gc, object, y, width, height);
			y += height;
			row = row.getNeighbor(ViewerRow.BELOW, false);
		}
		
		gc.setForeground(SharedColors.BORDER);
		gc.drawLine(width - 1, 0, width - 1, getClientArea().height);
	}
	
	/**
	 * @param object
	 * @param y
	 */
	private void drawObject(GC gc, GenericObject object, int y, int width, int height)
	{
		final int status = object.getStatus();
		
		if ((status == Severity.NORMAL) && !showNormal)
			return;
		if ((status == Severity.UNMANAGED) && !showUnmanaged)
			return;
		if ((status == Severity.UNKNOWN) && !showUnknown)
			return;
		if ((status == Severity.DISABLED) && !showDisabled)
			return;
		
		if (showIcons)
		{
			gc.drawImage(StatusDisplayInfo.getStatusImage(status), (width - 16) / 2, y + (height - 16) / 2);
		}
		else
		{
			gc.setBackground(StatusDisplayInfo.getStatusColor(status));
			gc.fillOval(4, y + 4, width - 8, height - 8);
		}
	}

	/**
	 * Refresh control
	 * 
	 * @param objectTree
	 */
	public void refresh(ObjectTreeViewer objectTree)
	{
		this.objectTree = objectTree;
		redraw();
	}
}
