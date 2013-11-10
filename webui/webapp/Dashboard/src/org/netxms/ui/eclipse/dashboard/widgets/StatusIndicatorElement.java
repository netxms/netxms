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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.dashboard.widgets.internal.StatusIndicatorConfig;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Status indicator
 */
public class StatusIndicatorElement extends ElementWidget
{
	private StatusIndicatorConfig config;
	private Canvas canvas;
	private Runnable refreshTimer;
	private Font font;
	private int status = Severity.UNKNOWN;
	private int xSize;
	private int ySize;

	private static final int MARGIN_X = 16;
	private static final int MARGIN_Y = 16;
	private static final int CIRCLE_SIZE = 36;

	/**
	 * @param parent
	 * @param element
	 */
	protected StatusIndicatorElement(final DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);

		try
		{
			config = StatusIndicatorConfig.createFromXml(element.getData());
		}
		catch(final Exception e)
		{
			e.printStackTrace();
			config = new StatusIndicatorConfig();
		}

		final FillLayout layout = new FillLayout();
		setLayout(layout);

		canvas = new Canvas(this, SWT.NONE);
		canvas.setBackground(colors.create(240, 240, 240));
		font = new Font(getDisplay(), "Verdana", 12, SWT.NONE); //$NON-NLS-1$

		calcSize(parent);

		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				font.dispose();
			}
		});

		canvas.addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e)
			{
				drawContent(e);
			}
		});

		startRefreshTimer();
	}

	/**
	 * @param parent
	 */
	private void calcSize(final DashboardControl parent)
	{
		final GC gc = new GC(parent);
		gc.setFont(font);
		final Point textExtent = gc.textExtent(config.getTitle());
		gc.dispose();
		xSize = (MARGIN_X * 3) + CIRCLE_SIZE + textExtent.x;
		ySize = (MARGIN_Y * 2) + CIRCLE_SIZE;
	}

	/**
	 * 
	 */
	protected void refreshData()
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final AbstractObject object = session.findObjectById(config.getObjectId());
		if (object != null)
		{
			status = object.getStatus();
		}
		else
		{
			status = Severity.UNKNOWN;
		}
		canvas.redraw();
	}

	/**
	 * 
	 */
	protected void startRefreshTimer()
	{
		final Display display = getDisplay();
		refreshTimer = new Runnable() {
			@Override
			public void run()
			{
				if (StatusIndicatorElement.this.isDisposed())
				{
					return;
				}

				refreshData();
				display.timerExec(1000, this);
			}
		};
		display.timerExec(1000, refreshTimer);
		refreshData();
	}

	/**
	 * @param e
	 */
	public void drawContent(PaintEvent e)
	{
		e.gc.setAntialias(SWT.ON);

		if (config.isFullColorRange())
		   e.gc.setBackground(StatusDisplayInfo.getStatusColor(status));
		else
         e.gc.setBackground((status == Severity.NORMAL) ? StatusDisplayInfo.getStatusColor(Severity.NORMAL) : StatusDisplayInfo.getStatusColor(Severity.CRITICAL));
      e.gc.fillOval(MARGIN_X, MARGIN_Y, CIRCLE_SIZE, CIRCLE_SIZE);

      e.gc.setBackground(canvas.getBackground());
      e.gc.setFont(font);
      final Point textExtent = e.gc.textExtent(config.getTitle());
      e.gc.drawText(config.getTitle(), (MARGIN_X * 2) + CIRCLE_SIZE, MARGIN_Y + (CIRCLE_SIZE / 2) - (textExtent.y / 2));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		return new Point(xSize, ySize);
	}
}
