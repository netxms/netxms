/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Canvas;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.StatusIndicatorConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Status indicator
 */
public class StatusIndicatorElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(StatusIndicatorElement.class);

   private final I18n i18n = LocalizationHelper.getI18n(StatusIndicatorElement.class);

	private StatusIndicatorConfig config;
	private Canvas canvas;
	private ViewRefreshController refreshController;
	private Font font;
	private ObjectStatus status = ObjectStatus.UNKNOWN;

	private static final int MARGIN_X = 16;
	private static final int MARGIN_Y = 16;
	private static final int CIRCLE_SIZE = 36;

	/**
	 * @param parent
	 * @param element
	 */
   protected StatusIndicatorElement(final DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, element, view);

		try
		{
			config = StatusIndicatorConfig.createFromXml(element.getData());
		}
		catch(final Exception e)
		{
         logger.error("Cannot parse dashboard element configuration", e);
			config = new StatusIndicatorConfig();
		}

      processCommonSettings(config);

      canvas = new Canvas(getContentArea(), SWT.DOUBLE_BUFFERED) {
         @Override
         public Point computeSize(int wHint, int hHint, boolean changed)
         {
            return new Point(MARGIN_X * 2 + CIRCLE_SIZE, MARGIN_Y * 2 + CIRCLE_SIZE);
         }
      };
		canvas.setBackground(colors.create(240, 240, 240));
		font = new Font(getDisplay(), "Verdana", 12, SWT.NONE); //$NON-NLS-1$

		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				font.dispose();
            refreshController.dispose();
			}
		});

		canvas.addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e)
			{
				drawContent(e);
			}
		});

      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Synchronize objects"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<Long> relatedOpbjects = new ArrayList<Long>();
            relatedOpbjects.add(getEffectiveObjectId(config.getObjectId()));
            session.syncMissingObjects(relatedOpbjects, true, NXCSession.OBJECT_SYNC_WAIT);

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refreshData();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot synchronize objects");
         }
      };
      job.setUser(false);
      job.start();

		startRefreshTimer();
	}

	/**
	 * Refresh element content
	 */
	protected void refreshData()
	{
      final NXCSession session = Registry.getSession();
      final AbstractObject object = session.findObjectById(getEffectiveObjectId(config.getObjectId()));
		if (object != null)
		{
			status = object.getStatus();
		}
		else
		{
			status = ObjectStatus.UNKNOWN;
		}
		canvas.redraw();
	}

	/**
	 * Start element refresh timer
	 */
	protected void startRefreshTimer()
	{
      refreshController = new ViewRefreshController(view, 1, new Runnable() {
			@Override
			public void run()
			{
				if (StatusIndicatorElement.this.isDisposed())
					return;

				refreshData();
			}
		});
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
         e.gc.setBackground((status == ObjectStatus.NORMAL) ? StatusDisplayInfo.getStatusColor(ObjectStatus.NORMAL) : StatusDisplayInfo.getStatusColor(ObjectStatus.CRITICAL));
      e.gc.fillOval(MARGIN_X, MARGIN_Y, CIRCLE_SIZE, CIRCLE_SIZE);
	}
}
