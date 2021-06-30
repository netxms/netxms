/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.charts.widgets;

import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.NetworkMap;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.Messages;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.DataChart;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Abstract base class for charts
 */
public abstract class GenericChart extends Canvas implements DataChart
{
	protected String title = Messages.get().GenericChart_Title0;
	protected boolean titleVisible = false;
	protected boolean legendVisible = true;
	protected boolean displayIn3D = true;
	protected boolean useLogScale = false;
	protected boolean translucent = false;
	protected ChartColor[] palette = null;
	protected IPreferenceStore preferenceStore;
	protected int legendPosition;
   protected long drillDownObjectId = 0;
   
   private boolean mouseDown = false;

	/**
	 * Generic constructor.
	 * 
	 * @param parent parent composite
	 * @param style SWT control styles
	 */
	public GenericChart(Composite parent, int style)
	{
		super(parent, style);

		preferenceStore = Activator.getDefault().getPreferenceStore();
		createDefaultPalette();
		
      addMouseListener(new MouseListener() {
         @Override
         public void mouseDown(MouseEvent e)
         {
            if (e.button == 1)
               mouseDown = true;
         }

         @Override
         public void mouseUp(MouseEvent e)
         {
            if ((e.button == 1) && mouseDown)
            {
               mouseDown = false;
               if (drillDownObjectId != 0)
                  openDrillDownObject();
            }
         }

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            mouseDown = false;
         }
      });
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#getTitle()
	 */
	@Override
	public String getChartTitle()
	{
		return title;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isLegendVisible()
	 */
	@Override
	public boolean isLegendVisible()
	{
		return legendVisible;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isTitleVisible()
	 */
	@Override
	public boolean isTitleVisible()
	{
		return titleVisible;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setPalette(org.netxms.ui.eclipse.charts.api.ChartColor[])
	 */
	@Override
	public void setPalette(ChartColor[] colors)
	{
		palette = colors;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setPaletteEntry(int, org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setPaletteEntry(int index, ChartColor color)
	{
		try
		{
			palette[index] = color;
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#getPaletteEntry(int)
	 */
	@Override
	public ChartColor getPaletteEntry(int index)
	{
		try
		{
			return palette[index];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return null;
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#is3DModeEnabled()
	 */
	@Override
	public boolean is3DModeEnabled()
	{
		return displayIn3D;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isLogScaleEnabled()
	 */
	@Override
	public boolean isLogScaleEnabled()
	{
		return useLogScale;
	}

	/**
	 * Create default palette from preferences
	 */
	protected void createDefaultPalette()
	{
		palette = new ChartColor[MAX_CHART_ITEMS];
		for(int i = 0; i < MAX_CHART_ITEMS; i++)
		{
			palette[i] = ChartColor.getDefaultColor(i);
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLegendPosition(int)
	 */
	@Override
	public void setLegendPosition(int position)
	{
		legendPosition = position;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#getLegendPosition()
	 */
	@Override
	public int getLegendPosition()
	{
		return legendPosition;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isTranslucent()
	 */
	@Override
	public boolean isTranslucent()
	{
		return translucent;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setTranslucent(boolean)
	 */
	@Override
	public void setTranslucent(boolean translucent)
	{
		this.translucent = translucent;
	}

   
   /**
    * Get ID of drill-down object for this gauge (dashboard or network map)
    */
   public long getDrillDownObjectId()
   {
      return drillDownObjectId;
   }
   
   /**
    * Set ID of drill-down object for this gauge (dashboard or network map)
    * 
    * @param objectId ID of drill-down object or 0 to disable drill-down functionality
    */
   public void setDrillDownObjectId(long drillDownObjectId)
   {
      this.drillDownObjectId = drillDownObjectId;
      setCursor(getDisplay().getSystemCursor((drillDownObjectId != 0) ? SWT.CURSOR_HAND : SWT.CURSOR_ARROW));
   }
   
   /**
    * Open drill-down object
    */
   void openDrillDownObject()
   {
      AbstractObject object = ConsoleSharedData.getSession().findObjectById(drillDownObjectId);
      if (object == null)
         return;
      
      if (!(object instanceof Dashboard) && !(object instanceof NetworkMap))
         return;
      
      final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
      try
      {
         window.getActivePage().showView(
               (object instanceof Dashboard) ? "org.netxms.ui.eclipse.dashboard.views.DashboardView" : "org.netxms.ui.eclipse.networkmaps.views.PredefinedMap",
                     Long.toString(object.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
      }
      catch(PartInitException e)
      {
         MessageDialogHelper.openError(window.getShell(), "Error", 
               String.format("Cannot open %s view \"%s\" (%s)",
                     (object instanceof Dashboard) ? "dashboard" : "network map",
                     object.getObjectName(), e.getMessage()));
      }
   }
}
