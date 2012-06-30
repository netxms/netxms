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
package org.netxms.ui.eclipse.objectview.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.shared.SharedColors;

/**
 * Widget representing node status
 */
public class NodeStatusWidget extends Canvas implements PaintListener
{
	private Node node;
	
	/**
	 * @param parent
	 */
	public NodeStatusWidget(Composite parent, Node node)
	{
		super(parent, SWT.NONE);
		this.node = node;
		addPaintListener(this);
		setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		Rectangle rect = getClientArea();
		rect.width--;
		rect.height--;
		
		e.gc.setAntialias(SWT.ON);
		e.gc.setTextAntialias(SWT.ON);
		e.gc.setForeground(SharedColors.BLACK);
		e.gc.setLineWidth(1);

		e.gc.setBackground(StatusDisplayInfo.getStatusColor(node.getStatus()));
		e.gc.setAlpha(127);
		e.gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
		e.gc.setAlpha(255);
		e.gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
		
		final String text = node.getObjectName() + "\n" + node.getPrimaryIP().getHostAddress();
		
		rect.x += 4;
		rect.y += 4;
		rect.width -= 8;
		rect.height -= 8;
		e.gc.setClipping(rect);
		int h = e.gc.textExtent(text, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER).y;
		e.gc.drawText(text, rect.x, rect.y + (rect.height - h) / 2, SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		GC gc = new GC(getShell());
		int h = gc.textExtent("MMM\nMMM", SWT.DRAW_TRANSPARENT | SWT.DRAW_DELIMITER).y;
		gc.dispose();
		return new Point(160, h + 8);
	}
}
