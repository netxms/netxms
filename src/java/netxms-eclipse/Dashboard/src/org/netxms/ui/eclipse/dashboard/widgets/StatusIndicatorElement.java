package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.dashboard.widgets.internal.StatusIndicatorConfig;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class StatusIndicatorElement extends ElementWidget
{
	private StatusIndicatorConfig config;
	private Canvas canvas;
	private Runnable refreshTimer;
	private Font font;
	private boolean greenState = false;
	private int xSize;
	private int ySize;

	private static final int MARGIN_X = 16;
	private static final int MARGIN_Y = 16;
	private static final int CIRCLE_SIZE = 36;

	protected StatusIndicatorElement(final DashboardControl parent, DashboardElement element)
	{
		super(parent, element);

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
		canvas.setBackground(new Color(getDisplay(), 240, 240, 240));
		font = new Font(getDisplay(), "Verdana", 12, SWT.NONE);

		calcSize(parent);

		addDisposeListener(new DisposeListener()
		{
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				font.dispose();
			}
		});

		canvas.addPaintListener(new PaintListener()
		{
			@Override
			public void paintControl(PaintEvent e)
			{
				drawContent(e);
			}
		});

		startRefreshTimer();
	}

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
		final GenericObject object = session.findObjectById(config.getObjectId());
		if (object != null)
		{
			greenState = object.getStatus() == GenericObject.STATUS_NORMAL;
		}
		else
		{
			greenState = false;
		}
		canvas.redraw();
	}

	protected void startRefreshTimer()
	{
		final Display display = getDisplay();
		refreshTimer = new Runnable()
		{
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

	public void drawContent(PaintEvent e)
	{
		e.gc.setAntialias(SWT.ON);

		Canvas canvas = (Canvas)e.widget;
		canvas.drawBackground(e.gc, 0, 0, 100, 100);

		final Color bgColor = canvas.getBackground();

		final Color redColors[] = { new Color(getDisplay(), 134, 0, 0), new Color(getDisplay(), 192, 0, 0) };
		final Color greenColors[] = { new Color(getDisplay(), 0, 134, 0), new Color(getDisplay(), 0, 192, 0) };

		if (greenState)
		{
			drawElement(e, MARGIN_X, MARGIN_Y, CIRCLE_SIZE, bgColor, greenColors, config.getTitle());
		}
		else
		{
			drawElement(e, MARGIN_X, MARGIN_Y, CIRCLE_SIZE, bgColor, redColors, config.getTitle());
		}

		redColors[0].dispose();
		redColors[1].dispose();
		greenColors[0].dispose();
		greenColors[1].dispose();
		bgColor.dispose();
	}

	private void drawElement(PaintEvent e, int xMargin, int yOffset, int size, final Color bgColor, final Color[] circleColors,
			final String label)
	{
		e.gc.setBackground(circleColors[0]);
		e.gc.fillOval(xMargin, yOffset, size, size);

		e.gc.setBackground(circleColors[1]);
		e.gc.fillOval(xMargin + 2, yOffset + 2, size - 4, size - 4);

		e.gc.setBackground(bgColor);
		e.gc.setFont(font);
		final Point textExtent = e.gc.textExtent(label);
		e.gc.drawText(label, (xMargin * 2) + size, yOffset + (size / 2) - (textExtent.y / 2));
	}

	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		return new Point(xSize, ySize);
	}
}
