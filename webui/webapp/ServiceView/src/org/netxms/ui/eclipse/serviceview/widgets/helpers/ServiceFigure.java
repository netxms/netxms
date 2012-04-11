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
package org.netxms.ui.eclipse.serviceview.widgets.helpers;

import org.eclipse.draw2d.BorderLayout;
import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.MouseEvent;
import org.eclipse.draw2d.MouseListener;
import org.eclipse.draw2d.PositionConstants;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Display;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;

/**
 * Figure which represents service in a service tree
 *
 */
public class ServiceFigure extends Figure
{
	private static final int INNER_SPACING = 5;
	private static final int HORIZONTAL_MARGIN = 4;
	private static final int VERTICAL_MARGIN = 4;
	private static final int MAX_LABEL_WIDTH = 70;
	
	private static final Color BORDER_COLOR = new Color(Display.getCurrent(), 0, 0, 0);
	private static final Color NORMAL_COLOR_LEFT = new Color(Display.getCurrent(), 164, 196, 212);
	private static final Color NORMAL_COLOR_RIGHT = new Color(Display.getCurrent(), 180, 208, 228);
	private static final Color SELECTION_COLOR_LEFT = new Color(Display.getCurrent(), 255, 242, 0);
	private static final Color SELECTION_COLOR_RIGHT = new Color(Display.getCurrent(), 225, 212, 0);
	
	private ServiceTreeElement service;
	private ServiceTreeLabelProvider labelProvider;
	private IServiceFigureListener listener;
	private Label expandBox;
	private Label label;
	
	public ServiceFigure(ServiceTreeElement service, ServiceTreeLabelProvider labelProvider, IServiceFigureListener listener)
	{
		this.service = service;
		this.labelProvider = labelProvider;
		this.listener = listener;
		
		label = new Label(service.getObject().getObjectName());
		label.setFont(labelProvider.getFont());
		label.setLabelAlignment(PositionConstants.CENTER);
		label.setIcon(labelProvider.getImage(service));
		label.setMaximumSize(new Dimension(MAX_LABEL_WIDTH, -1));
		add(label, BorderLayout.LEFT);
		
		expandBox = new Label("");
		expandBox.setFont(labelProvider.getFont());
		expandBox.setIcon(labelProvider.getExpansionStatusIcon(service));
		expandBox.setIconAlignment(PositionConstants.LEFT);
		add(expandBox, BorderLayout.RIGHT);
		
		if (listener != null)
		{
			label.addMouseListener(new MouseListener() {
				@Override
				public void mouseDoubleClicked(MouseEvent me)
				{
					if (me.button == 1)
						ServiceFigure.this.listener.expandButtonPressed(ServiceFigure.this.service);
				}

				@Override
				public void mousePressed(MouseEvent me)
				{
				}

				@Override
				public void mouseReleased(MouseEvent me)
				{
				}
			});
			
			expandBox.addMouseListener(new MouseListener() {
				@Override
				public void mouseDoubleClicked(MouseEvent me)
				{
				}
	
				@Override
				public void mousePressed(MouseEvent me)
				{
				}
	
				@Override
				public void mouseReleased(MouseEvent me)
				{
					if (me.button == 1)
						ServiceFigure.this.listener.expandButtonPressed(ServiceFigure.this.service);
				}
			});
		}
		
		Dimension ls = label.getPreferredSize(MAX_LABEL_WIDTH, -1);
		Dimension es = expandBox.getPreferredSize(-1, -1);
		label.setSize(ls);
		expandBox.setSize(es);
		int width = ls.width + es.width + INNER_SPACING + HORIZONTAL_MARGIN * 2;
		int height = ls.height + VERTICAL_MARGIN * 2;
		setSize(width, height);
		label.setLocation(new Point(HORIZONTAL_MARGIN, VERTICAL_MARGIN));
		expandBox.setLocation(new Point(width - HORIZONTAL_MARGIN - es.width, height / 2 - es.height / 2));
		
		//setToolTip(new ObjectTooltip(object));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected void paintFigure(Graphics gc)
	{
		Rectangle rect = new Rectangle(getBounds());
		rect.height--;
		rect.width--;
		gc.setAntialias(SWT.ON);

		// Selection mark or status-based background
		if (labelProvider.isElementSelected(service))
		{
			gc.setBackgroundColor(SELECTION_COLOR_LEFT);
			/*
			Pattern pattern = new Pattern(Display.getDefault(), rect.x, rect.y, rect.x + rect.width, rect.y + rect.height, SELECTION_COLOR_LEFT, SELECTION_COLOR_RIGHT);
			gc.setBackgroundPattern(pattern);
			gc.fillRoundRectangle(rect, 8, 8);
			pattern.dispose();
			*/
		}
		else
		{
			gc.setBackgroundColor(StatusDisplayInfo.getStatusColor(service.getObject().getStatus()));
			gc.setAlpha(32);
			/*
			Pattern pattern = new Pattern(Display.getDefault(), rect.x, rect.y, rect.x + rect.width, rect.y + rect.height, NORMAL_COLOR_LEFT, NORMAL_COLOR_RIGHT);
			gc.setBackgroundPattern(pattern);
			gc.fillRoundRectangle(rect, 8, 8);
			*/
			gc.setAlpha(255);
		}
		
		// Border
		gc.setLineWidth(1);
		gc.setForegroundColor(BORDER_COLOR);		
		gc.drawRoundRectangle(rect, 8, 8);
	}
}
