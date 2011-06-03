package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ConditionInfo;
import org.netxms.ui.eclipse.dashboard.widgets.internal.StatusIndicatorConfig;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class StatusIndicatorElement extends ElementWidget
{
	private StatusIndicatorConfig config;
	private Runnable refreshTimer;
	private Font font;

	protected StatusIndicatorElement(final Composite parent, final String data, String elementLayout)
	{
		super(parent, data, elementLayout);

		try
		{
			config = StatusIndicatorConfig.createFromXml(data);
		}
		catch(final Exception e)
		{
			e.printStackTrace();
			config = new StatusIndicatorConfig();
		}

		final FillLayout layout = new FillLayout();
		setLayout(layout);

		final Canvas canvas = new Canvas(this, SWT.NONE);
		canvas.setBackground(new Color(getDisplay(), 240, 240, 240));
		font = new Font(getDisplay(), "Verdana", 12, SWT.NONE);

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

	protected void refreshData()
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();

		final ConditionInfo[] conditions = config.getConditionList();
		for(ConditionInfo condition : conditions)
		{
			final GenericObject object = session.findObjectById(condition.getId());
			if (object instanceof Condition)
			{
				if (((Condition)object).getStatus() != Condition.STATUS_NORMAL)
				{
					condition.setActive(true);
				}
				else
				{
					condition.setActive(false);
				}
			}
		}

		redraw();
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
				display.timerExec(30000, this);
			}
		};
		display.timerExec(30000, refreshTimer);
		refreshData();
	}

	public void drawContent(PaintEvent e)
	{
		e.gc.setAntialias(SWT.ON);

		Canvas canvas = (Canvas)e.widget;
		canvas.drawBackground(e.gc, 0, 0, 100, 100);

		int xMargin = 16;
		int yMargin = 16;
		int yOffset = yMargin;
		int size = 36;

		//final Color bgColor = new Color(getDisplay(), 255, 255, 255);
		final Color bgColor = canvas.getBackground();
		
		final Color redColors[] = { new Color(getDisplay(), 134, 0, 0), new Color(getDisplay(), 192, 0, 0) };
		final Color greenColors[] = { new Color(getDisplay(), 0, 134, 0), new Color(getDisplay(), 0, 192, 0) };

		final ConditionInfo[] conditions = config.getConditionList();
		for(ConditionInfo condition : conditions)
		{
			if (condition.isActive())
			{
				drawElement(e, xMargin, yOffset, size, bgColor, redColors, condition.getName());
			}
			else
			{
				drawElement(e, xMargin, yOffset, size, bgColor, greenColors, condition.getName());
			}
			yOffset += yMargin + size;
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
}
