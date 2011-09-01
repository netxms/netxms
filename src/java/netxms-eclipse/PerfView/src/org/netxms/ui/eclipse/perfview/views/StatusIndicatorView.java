package org.netxms.ui.eclipse.perfview.views;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.util.ArrayList;
import java.util.List;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

public class StatusIndicatorView extends ViewPart
{

	private class Element
	{
		long nodeId;
		long dciId;
		String name;
		boolean thresholdActive;
		Canvas canvas;

		public Element(long nodeId, long dciId, String name)
		{
			this.nodeId = nodeId;
			this.dciId = dciId;
			this.name = name;
		}

	};

	public static final String ID = "org.netxms.ui.eclipse.perfview.views.StatusIndicatorView";

	private Runnable refreshTimer;
	private int autoRefreshInterval = 30000; // 30 seconds

	private NXCSession session;

	private List<Element> items = new ArrayList<Element>();

	private boolean updateInProgress;

	private Image imageRed;

	private Image imageGreen;

	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);

		session = (NXCSession)ConsoleSharedData.getSession();

		String id = site.getSecondaryId();
		String[] fields = id.split("&");

		for(int i = 1; i < fields.length; i++)
		{
			String[] subfields = fields[i].split("\\@");
			if (subfields.length == 6)
			{
				try
				{
					final long nodeId = Long.parseLong(subfields[0], 10);
					final long dciId = Long.parseLong(subfields[1], 10);
					final String description = URLDecoder.decode(subfields[5], "UTF-8");

					items.add(new Element(nodeId, dciId, description));
				}
				catch(NumberFormatException e)
				{
					e.printStackTrace();
				}
				catch(UnsupportedEncodingException e)
				{
					e.printStackTrace();
				}
			}
		}

		if (items.size() == 1)
		{
			Element item = items.get(0);
			GenericObject object = session.findObjectById(item.nodeId);
			if (object != null)
			{
				setPartName(item.name);
			}
		}

		final Display display = site.getShell().getDisplay();
		refreshTimer = new Runnable()
		{
			@Override
			public void run()
			{
				update();
				display.timerExec(autoRefreshInterval, this);
			}
		};

		imageRed = Activator.getImageDescriptor("icons/light-red.png").createImage();
		imageGreen = Activator.getImageDescriptor("icons/light-green.png").createImage();
	}

	protected void update()
	{
		if (updateInProgress)
		{
			return;
		}

		updateInProgress = true;

		Job job = new Job("Get DCI values")
		{
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;

				try
				{
					for(Element item : items)
					{
						final Threshold[] thresholds = session.getThresholds(item.nodeId, item.dciId);
						item.thresholdActive = thresholds[0].isActive();
					}

					status = Status.OK_STATUS;

					new UIJob("Update chart")
					{

						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							for(Element item : items)
							{
								item.canvas.redraw();
							}
							updateInProgress = false;
							return Status.OK_STATUS;
						}
					}.schedule();
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID,
							(e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
							"Cannot get DCI values: " + e.getMessage(), e);
					updateInProgress = false;
				}

				return status;
			}

			/*
			 * (non-Javadoc)
			 * 
			 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
			 */
			@Override
			public boolean belongsTo(Object family)
			{
				return family == Activator.PLUGIN_ID;
			}
		};
		IWorkbenchSiteProgressService siteService = (IWorkbenchSiteProgressService)getSite().getAdapter(
				IWorkbenchSiteProgressService.class);
		siteService.schedule(job, 0, true);
	}

	@Override
	public void createPartControl(final Composite parent)
	{
		final GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		parent.setLayout(layout);

		layout.numColumns = 5;

		for(int i = 0; i < items.size(); i++)
		{
			Element item = items.get(i);
			createElement(parent, i, item);
		}

		getSite().getShell().getDisplay().timerExec(autoRefreshInterval, refreshTimer);

		update();
	}

	private void createElement(final Composite parent, final int itemId, final Element item)
	{
		final Canvas drawingCanvas = new Canvas(parent, SWT.NONE);
		final GridData data = new GridData(GridData.FILL_BOTH);
		drawingCanvas.setLayoutData(data);

		item.canvas = drawingCanvas;

		drawingCanvas.addPaintListener(new PaintListener()
		{

			@Override
			public void paintControl(PaintEvent e)
			{
				e.gc.setAntialias(SWT.ON);
				Canvas canvas = (Canvas)e.widget;

				final Point textExtent = e.gc.textExtent(item.name);

				final Rectangle clientArea = canvas.getClientArea();

				//int maxBlobSize = Math.min(clientArea.width, clientArea.height);

				e.gc.drawText(item.name, clientArea.width / 2 - textExtent.x / 2, clientArea.height - textExtent.y);

				final Image image;
				if (item.thresholdActive)
				{
					image = imageRed;
				}
				else
				{
					image = imageGreen;
				}
				final int x = (clientArea.width / 2) - (image.getBounds().width / 2);
				final int y = (clientArea.height / 2) - (image.getBounds().height / 2) - (textExtent.y / 2);
				e.gc.drawImage(image, x, y);

				/*
				 * final Color color = parent.getShell().getDisplay()
				 * .getSystemColor(item.thresholdActive ? SWT.COLOR_RED :
				 * SWT.COLOR_GREEN); e.gc.setBackground(color);
				 * 
				 * maxBlobSize -= textExtent.y; maxBlobSize -=
				 * WidgetHelper.INNER_SPACING;
				 * 
				 * final int x = (clientArea.width / 2) - (maxBlobSize / 2); final
				 * int y = (clientArea.height / 2) - (maxBlobSize / 2) -
				 * (textExtent.y / 2); e.gc.fillOval(x, y, maxBlobSize,
				 * maxBlobSize);
				 */
			}
		});
	}

	@Override
	public void setFocus()
	{
	}

}
