/**
 * 
 */
package org.netxms.ui.eclipse.slm.views;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.objects.ServiceContainer;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.ChartFactory;
import org.netxms.ui.eclipse.charts.api.DataComparisonChart;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedColors;
import org.netxms.ui.eclipse.slm.Messages;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Business service availability view
 */
public class ServiceAvailability extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.slm.views.ServiceAvailability"; //$NON-NLS-1$
	
	private ServiceContainer object;
	private DataComparisonChart dayChart;
	private DataComparisonChart weekChart;
	private DataComparisonChart monthChart;
	private ColorCache colors;
	private SessionListener listener;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		try
		{
			NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			object = (ServiceContainer)session.findObjectById(Long.parseLong(site.getSecondaryId()), ServiceContainer.class);
		}
		catch(Exception e)
		{
			throw new PartInitException(Messages.ServiceAvailability_InternalError, e);
		}
		if (object == null)
			throw new PartInitException(Messages.ServiceAvailability_InitErrorPart1 + site.getSecondaryId() + Messages.ServiceAvailability_InitErrorPart2);
		setPartName(Messages.ServiceAvailability_PartNamePrefix + object.getObjectName());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final Composite clientArea = new Composite(parent, SWT.NONE);
		colors = new ColorCache(clientArea);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 4;
		clientArea.setLayout(layout);
		
		dayChart = createChart(clientArea, Messages.ServiceAvailability_Today);
		weekChart = createChart(clientArea, Messages.ServiceAvailability_ThisWeek);
		monthChart = createChart(clientArea, Messages.ServiceAvailability_ThisMonth);
		
		Canvas legend = new Canvas(clientArea, SWT.NONE);
		legend.addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e)
			{
				paintLegend(e.gc);
			}
		});
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		legend.setLayoutData(gd);
		
		listener = new SessionListener() {
			@Override
			public void notificationHandler(final SessionNotification n)
			{
				if ((n.getCode() == NXCNotification.OBJECT_CHANGED) && (n.getSubCode() == object.getObjectId()))
				{
					clientArea.getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							object = (ServiceContainer)n.getObject();
							refresh();
						}
					});
				}
			}
		};
		ConsoleSharedData.getSession().addListener(listener);
	}

	/**
	 * @param parent
	 * @param title
	 * @return
	 */
	private DataComparisonChart createChart(Composite parent, String title)
	{
		DataComparisonChart chart = ChartFactory.createPieChart(parent, SWT.NONE);
		chart.setTitleVisible(true);
		chart.set3DModeEnabled(true);
		chart.setChartTitle(title);
		chart.setLegendVisible(false);
		chart.setLabelsVisible(true);
		chart.setRotation(225.0);
		
		chart.addParameter(new GraphItem(0, 0, 0, 0, Messages.ServiceAvailability_Up, Messages.ServiceAvailability_Up), 100);
		chart.addParameter(new GraphItem(0, 0, 0, 0, Messages.ServiceAvailability_Down, Messages.ServiceAvailability_Down), 0);
		chart.setPaletteEntry(0, new ChartColor(127, 154, 72));
		chart.setPaletteEntry(1, new ChartColor(158, 65, 62));
		chart.initializationComplete();
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		((Control)chart).setLayoutData(gd);
		return chart;
	}

	/**
	 * Paint legend for charts
	 * 
	 * @param gc
	 */
	private void paintLegend(GC gc)
	{
		int th = gc.textExtent(Messages.ServiceAvailability_UptimeDowntime).y;
		
		gc.setBackground(colors.create(127, 154, 72));
		gc.setForeground(ColorConverter.adjustColor(gc.getBackground(), SharedColors.BLACK, 0.2f, colors));
		gc.fillRectangle(5, 10, th, th);
		gc.drawRectangle(5, 10, th, th);

		gc.setBackground(colors.create(158, 65, 62));
		gc.setForeground(ColorConverter.adjustColor(gc.getBackground(), SharedColors.BLACK, 0.2f, colors));
		gc.fillRectangle(5, 40, th, th);
		gc.drawRectangle(5, 40, th, th);

		gc.setForeground(SharedColors.BLACK);
		gc.drawText(Messages.ServiceAvailability_Uptime, 10 + th, 10, true);
		gc.drawText(Messages.ServiceAvailability_Downtime, 10 + th, 40, true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
	}
	
	/**
	 * Refresh charts
	 */
	private void refresh()
	{
		dayChart.updateParameter(0, object.getUptimeForDay(), false);
		dayChart.updateParameter(1, 100.0 - object.getUptimeForDay(), true);

		weekChart.updateParameter(0, object.getUptimeForWeek(), false);
		weekChart.updateParameter(1, 100.0 - object.getUptimeForWeek(), true);
		
		monthChart.updateParameter(0, object.getUptimeForMonth(), false);
		monthChart.updateParameter(1, 100.0 - object.getUptimeForMonth(), true);
	}
}
