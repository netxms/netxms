/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import java.util.Map;
import java.util.regex.Pattern;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.maps.configs.MapDataSource;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.StatusIndicatorConfig;
import org.netxms.nxmc.modules.dashboards.config.StatusIndicatorConfig.StatusIndicatorElementConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;
import com.google.gson.Gson;

/**
 * Status indicator
 */
public class StatusIndicatorElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(StatusIndicatorElement.class);

   private final I18n i18n = LocalizationHelper.getI18n(StatusIndicatorElement.class);

   private NXCSession session = Registry.getSession();
	private StatusIndicatorConfig config;
	private ViewRefreshController refreshController;
   private boolean requireScriptRun = false;
   private boolean requireDataCollection = false;
   private StatusIndicatorElementWidget[] elementWidgets;

	private static final int ELEMENT_HEIGHT = 36;

	/**
	 * @param parent
	 * @param element
	 */
   protected StatusIndicatorElement(final DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, element, view);

		try
		{
         config = new Gson().fromJson(element.getData(), StatusIndicatorConfig.class);
		}
		catch(final Exception e)
		{
         logger.error("Cannot parse dashboard element configuration", e);
         logger.debug("Original XML:\n" + element.getData());
			config = new StatusIndicatorConfig();
		}

      processCommonSettings(config);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 16;
      layout.marginWidth = 16;
      layout.verticalSpacing = 16;
      layout.horizontalSpacing = 16;
      layout.numColumns = config.getNumColumns();
      layout.makeColumnsEqualWidth = true;
      getContentArea().setLayout(layout);

      elementWidgets = new StatusIndicatorElementWidget[config.getElements().length];
      for(int i = 0; i < elementWidgets.length; i++)
      {
         StatusIndicatorElementConfig e = config.getElements()[i];
         if (e.getType() == StatusIndicatorConfig.ELEMENT_TYPE_SCRIPT)
            requireScriptRun = true;
         else if ((e.getType() == StatusIndicatorConfig.ELEMENT_TYPE_DCI) || (e.getType() == StatusIndicatorConfig.ELEMENT_TYPE_DCI_TEMPLATE))
            requireDataCollection = true;
         elementWidgets[i] = new StatusIndicatorElementWidget(getContentArea(), e);
         elementWidgets[i].setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      }

      Job job = new Job(i18n.tr("Synchronize objects"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<Long> relatedObjects = new ArrayList<Long>();
            for(StatusIndicatorElementConfig e : config.getElements())
            {
               if (e.getType() == StatusIndicatorConfig.ELEMENT_TYPE_OBJECT)
                  relatedObjects.add(getEffectiveObjectId(e.getObjectId()));
            }
            if (!relatedObjects.isEmpty())
            {
               session.syncMissingObjects(relatedObjects, NXCSession.OBJECT_SYNC_WAIT);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (!isDisposed())
                        refreshData();
                  }
               });
            }

            // Resolve DCI names
            DciValue[] nodeDciList = null;
            for(int i = 0; i < elementWidgets.length; i++)
            {
               StatusIndicatorElementConfig e = config.getElements()[i];
               if (e.getType() == StatusIndicatorConfig.ELEMENT_TYPE_DCI_TEMPLATE)
               {
                  AbstractObject contextObject = getContext();
                  if (contextObject == null)
                     break;

                  if (nodeDciList == null)
                     nodeDciList = session.getLastValues(contextObject.getObjectId());

                  Pattern namePattern = !e.getDciName().isEmpty() ? Pattern.compile(e.getDciName()) : null;
                  Pattern descriptionPattern = !e.getDciDescription().isEmpty() ? Pattern.compile(e.getDciDescription()) : null;
                  for(DciValue dciInfo : nodeDciList)
                  {
                     if (((namePattern != null) && namePattern.matcher(dciInfo.getName()).find()) || ((descriptionPattern != null) && descriptionPattern.matcher(dciInfo.getDescription()).find()))
                     {
                        e.setObjectId(contextObject.getObjectId());
                        e.setDciId(dciInfo.getId());
                        break;
                     }
                  }
               }
            }
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

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            refreshController.dispose();
         }
      });
	}

	/**
	 * Refresh element content
	 */
   private void refreshData()
	{
      if (requireDataCollection || requireScriptRun)
      {
         Job job = new Job(i18n.tr("Update status indicator"), view) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               final Map<String, String> scriptData;
               if (requireScriptRun)
               {
                  long contextObjectId = config.getScriptContextObjectId();
                  if (contextObjectId == 0)
                     contextObjectId = getDashboardObjectId();
                  else if (contextObjectId == AbstractObject.CONTEXT)
                     contextObjectId = getContextObjectId();
                  scriptData = session.queryScript(contextObjectId, config.getScript(), null, null);
               }
               else
               {
                  scriptData = null;
               }

               final DciValue[] dciValues;
               if (requireDataCollection)
               {
                  List<MapDataSource> dciList = new ArrayList<>();
                  for(int i = 0; i < config.getElements().length; i++)
                  {
                     StatusIndicatorElementConfig e = config.getElements()[i];
                     if (((e.getType() == StatusIndicatorConfig.ELEMENT_TYPE_DCI) || (e.getType() == StatusIndicatorConfig.ELEMENT_TYPE_DCI_TEMPLATE)) && (e.getDciId() != 0))
                     {
                        dciList.add(new MapDataSource(e.getObjectId(), e.getDciId()));
                     }
                  }
                  dciValues = !dciList.isEmpty() ? session.getLastValues(dciList) : null;
               }
               else
               {
                  dciValues = null;
               }

               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (!isDisposed())
                        updateElements(scriptData, dciValues);
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return "Error updating status indicator";
            }
         };
         job.setUser(false);
         job.start();
      }
      else
      {
         updateElements(null, null);
      }
   }

   /**
    * Update elements
    *
    * @param scriptData data from script
    * @param dciValues collected DCI values
    */
   private void updateElements(Map<String, String> scriptData, DciValue[] dciValues)
   {
      for(StatusIndicatorElementWidget w : elementWidgets)
      {
         StatusIndicatorElementConfig e = w.getElementConfig();
         switch(e.getType())
         {
            case StatusIndicatorConfig.ELEMENT_TYPE_OBJECT:
               final AbstractObject object = session.findObjectById(getEffectiveObjectId(e.getObjectId()));
               w.setStatus((object != null) ? object.getStatus() : ObjectStatus.UNKNOWN);
               break;
            case StatusIndicatorConfig.ELEMENT_TYPE_SCRIPT:
               String value = scriptData.get(e.getTag());
               if (value != null)
               {
                  try
                  {
                     w.setStatus(ObjectStatus.getByValue(Integer.parseInt(value)));
                  }
                  catch(NumberFormatException ex)
                  {
                     w.setStatus(ObjectStatus.UNKNOWN);
                  }
               }
               else
               {
                  w.setStatus(ObjectStatus.UNKNOWN);
               }
               break;
            case StatusIndicatorConfig.ELEMENT_TYPE_DCI:
            case StatusIndicatorConfig.ELEMENT_TYPE_DCI_TEMPLATE:
               boolean found = false;
               if (dciValues != null)
               {
                  for(DciValue v : dciValues)
                  {
                     if (v.getId() == e.getDciId())
                     {
                        Threshold t = v.getActiveThreshold();
                        w.setStatus((t != null) ? ObjectStatus.getByValue(t.getCurrentSeverity().getValue()) : ObjectStatus.NORMAL);
                        found = true;
                        break;
                     }
                  }
               }
               if (!found)
               {
                  w.setStatus(ObjectStatus.UNKNOWN);
               }
               break;
         }
      }
   }

	/**
	 * Start element refresh timer
	 */
	protected void startRefreshTimer()
	{
      refreshController = new ViewRefreshController(view, 15, new Runnable() {
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
    * Widget that draws single status indicator element
    */
   private class StatusIndicatorElementWidget extends Canvas
   {
      private StatusIndicatorElementConfig elementConfig;
      private ObjectStatus status;

      /**
       * Create new status indicator element wiodget
       *
       * @param parent parent composite
       * @param elementConfig element to draw
       */
      StatusIndicatorElementWidget(Composite parent, StatusIndicatorElementConfig elementConfig)
      {
         super(parent, SWT.NONE);
         this.elementConfig = elementConfig;
         status = ObjectStatus.UNKNOWN;
         addPaintListener(new PaintListener() {
            @Override
            public void paintControl(PaintEvent e)
            {
               drawContent(e.gc);
            }
         });
      }

      /**
       * Draw element content
       *
       * @param gc
       */
      private void drawContent(GC gc)
      {
         gc.setAntialias(SWT.ON);
         if (config.isFullColorRange())
            gc.setBackground(StatusDisplayInfo.getStatusColor(status));
         else
            gc.setBackground((status == ObjectStatus.NORMAL) ? StatusDisplayInfo.getStatusColor(ObjectStatus.NORMAL) : StatusDisplayInfo.getStatusColor(ObjectStatus.CRITICAL));

         Rectangle clientArea = getClientArea();
         Rectangle indicatorRect;
         switch(config.getLabelType())
         {
            case StatusIndicatorConfig.LABEL_INSIDE:
               indicatorRect = new Rectangle(0, (clientArea.height - ELEMENT_HEIGHT) / 2, clientArea.width, ELEMENT_HEIGHT);
               break;
            case StatusIndicatorConfig.LABEL_OUTSIDE:
               indicatorRect = new Rectangle(0, (clientArea.height - ELEMENT_HEIGHT) / 2, ELEMENT_HEIGHT, ELEMENT_HEIGHT);
               break;
            case StatusIndicatorConfig.LABEL_NONE:
               indicatorRect = new Rectangle((clientArea.width - ELEMENT_HEIGHT) / 2, (clientArea.height - ELEMENT_HEIGHT) / 2, ELEMENT_HEIGHT, ELEMENT_HEIGHT);
               break;
            default:
               return; // invalid configuration
         }

         switch(config.getShape())
         {
            case StatusIndicatorConfig.SHAPE_CIRCLE:
               gc.fillOval(indicatorRect.x, indicatorRect.y, indicatorRect.width, indicatorRect.height);
               break;
            case StatusIndicatorConfig.SHAPE_RECTANGLE:
               gc.fillRectangle(indicatorRect);
               break;
            case StatusIndicatorConfig.SHAPE_ROUNDED_RECTANGLE:
               gc.fillRoundRectangle(indicatorRect.x, indicatorRect.y, indicatorRect.width, indicatorRect.height, 8, 8);
               break;
         }

         if (config.getLabelType() != StatusIndicatorConfig.LABEL_NONE)
         {
            gc.setFont(JFaceResources.getBannerFont());

            String label = elementConfig.getLabel();
            if (((label == null) || label.isEmpty()) && (elementConfig.getType() == StatusIndicatorConfig.ELEMENT_TYPE_OBJECT))
            {
               label = session.getObjectName(getEffectiveObjectId(elementConfig.getObjectId()));
               elementConfig.setLabel(label);
            }
            Point textExtent = gc.textExtent(label);
            if (config.getLabelType() == StatusIndicatorConfig.LABEL_INSIDE)
            {
               gc.setForeground(getDisplay().getSystemColor(ColorConverter.isDarkColor(gc.getBackground()) ? SWT.COLOR_GRAY : SWT.COLOR_BLACK));
               gc.drawText(elementConfig.getLabel(), (clientArea.width - textExtent.x) / 2, (clientArea.height - textExtent.y) / 2);
            }
            else
            {
               gc.setBackground(getBackground());
               gc.drawText(elementConfig.getLabel(), ELEMENT_HEIGHT + 4, (clientArea.height - textExtent.y) / 2);
            }
         }
      }

      /**
       * @see org.eclipse.swt.widgets.Control#computeSize(int, int, boolean)
       */
      @Override
      public Point computeSize(int wHint, int hHint, boolean changed)
      {
         return new Point((wHint == SWT.DEFAULT) ? ELEMENT_HEIGHT : wHint, ELEMENT_HEIGHT);
      }

      /**
       * @return the elementConfig
       */
      public StatusIndicatorElementConfig getElementConfig()
      {
         return elementConfig;
      }

      /**
       * Set new status and redraw.
       *
       * @param status new status
       */
      public void setStatus(ObjectStatus status)
      {
         if (this.status != status)
         {
            this.status = status;
            redraw();
         }
      }
   }
}
