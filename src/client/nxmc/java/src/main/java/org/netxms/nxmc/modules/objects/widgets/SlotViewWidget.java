/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.objects.widgets;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.topology.Port;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortCalculator;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortCalculatorDownUpLeftRight;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortCalculatorLeftRightDownUp;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortCalculatorLeftRightUpDown;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortCalculatorUpDownLeftRight;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortInfo;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortSelectionListener;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Widget for displaying single slot in device view
 */
public class SlotViewWidget extends Canvas implements PaintListener, MouseListener, MouseTrackListener
{
   // Slot layouts
   private static final int NDD_PN_UNKNOWN = 0; // port layout not known to driver, default used (layout 4)
   private static final int NDD_PN_CUSTOM = 1;  // custom layout, driver defines location of each port
   private static final int NDD_PN_LR_UD = 2;   // left-to-right, then up-down:
                                                //  1 2 3 4
                                                //  5 6 7 8
   private static final int NDD_PN_LR_DU = 3;   // left-to-right, then down-up:
                                                //  5 6 7 8
                                                //  1 2 3 4
   private static final int NDD_PN_UD_LR = 4;   // up-down, then left-right:
                                                //  1 3 5 7
                                                //  2 4 6 8
   private static final int NDD_PN_DU_LR = 5;   // down-up, then left-right:
                                                //  2 4 6 8
                                                //  1 3 5 7
   
	private static final int HORIZONTAL_MARGIN = 20;
	private static final int VERTICAL_MARGIN = 10;
	private static final int PORT_WIDTH = 44;
	private static final int PORT_HEIGHT = 30;

	public static final int DISPLAY_MODE_NONE = 0;
   public static final int DISPLAY_MODE_STATE = 1;
   public static final int DISPLAY_MODE_STATUS = 2;

	private List<PortInfo> ports = new ArrayList<PortInfo>();
	private int rowCount;
	private int numberingScheme;
	private String slotName;
	private Point nameSize;
	private int displayMode = DISPLAY_MODE_NONE;
	private PortInfo selection = null;
	private Set<PortSelectionListener> selectionListeners = new HashSet<PortSelectionListener>();
	private ColorCache colors;
	private PortLocationFinder finder = new PortLocationFinder();
   private AbstractObject tooltipObject = null;
   private ObjectPopupDialog tooltipDialog = null;
   private RGB colorBackground = ThemeEngine.getBackgroundColorDefinition("DeviceView.Port");
   private RGB colorHighlight = ThemeEngine.getBackgroundColorDefinition("DeviceView.PortHighlight");

	/**
	 * @param parent
	 * @param style
	 */
	public SlotViewWidget(Composite parent, int style, String slotName, int rowCount, int numberingScheme)
	{
		super(parent, style | SWT.BORDER | SWT.DOUBLE_BUFFERED);
		this.slotName = slotName;
		this.numberingScheme = numberingScheme;
		if (rowCount == 0)
		   this.rowCount = 2; // For when row count is not yet received from driver
		else
		   this.rowCount = rowCount;

		colors = new ColorCache(this);

		GC gc = new GC(getDisplay());
		nameSize = gc.textExtent(slotName);
		gc.dispose();

		addPaintListener(this);
		addMouseListener(this);
      WidgetHelper.attachMouseTrackListener(this, this);
	}

   /**
    * @see org.eclipse.swt.widgets.Widget#dispose()
    */
	@Override
   public void dispose()
   {
      removePaintListener(this);
      super.dispose();
   }

   /**
    * Add port to view
    *
    * @param p port information
    */
	public void addPort(PortInfo p)
	{
		ports.add(p);
	}

   /**
    * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
    */
	@Override
	public void paintControl(PaintEvent e)
	{
		e.gc.drawText(slotName, HORIZONTAL_MARGIN, (getSize().y - nameSize.y) / 2);

      int pic = 0;
      Point pos = null;
      PortCalculator portCalculator = createPortCalculator(nameSize.x);
		for(PortInfo p : ports)
		{
         if (p.getPIC() != pic)
         {
            pic = p.getPIC();
            if (pos != null)
               portCalculator = createPortCalculator(pos.x + PORT_WIDTH);
         }
         pos = portCalculator.calculateNextPos();
         drawPort(p, pos, e.gc);
		}
	}

   /**
    * Create new port calculator based on numbering scheme
    *
    * @param baseX base X coordinate
    * @return port calculator
    */
   private PortCalculator createPortCalculator(int baseX)
   {
      switch(numberingScheme)
      {
         case NDD_PN_DU_LR:
            return new PortCalculatorDownUpLeftRight(baseX, rowCount);
         case NDD_PN_LR_UD:
            return new PortCalculatorLeftRightUpDown(baseX, ports.size(), rowCount);
         case NDD_PN_LR_DU:
            return new PortCalculatorLeftRightDownUp(baseX, ports.size(), rowCount);
         case NDD_PN_CUSTOM:
         case NDD_PN_UNKNOWN:
         case NDD_PN_UD_LR:
         default:
            return new PortCalculatorUpDownLeftRight(baseX, rowCount);
      }
   }

	/**
	 * Draw single port
	 * 
	 * @param p port information
	 * @param x X coordinate of top left corner
	 * @param y Y coordinate of top left corner
	 */
	private void drawPort(PortInfo p, Point point, GC gc)
	{
		final String label = Integer.toString(p.getPort());
		Rectangle rect = new Rectangle(point.x, point.y, PORT_WIDTH, PORT_HEIGHT);

		finder.addPortLocation(rect, p);

		if (displayMode == DISPLAY_MODE_STATE)
		{
			ObjectStatus status = ObjectStatus.UNKNOWN;
			switch(p.getAdminState())
			{
				case Interface.ADMIN_STATE_DOWN:
					status = ObjectStatus.DISABLED;
					break;
				case Interface.ADMIN_STATE_TESTING:
					status = ObjectStatus.TESTING;
					break;
				case Interface.ADMIN_STATE_UP:
					switch(p.getOperState())
					{
						case Interface.OPER_STATE_DOWN:
							status = ObjectStatus.CRITICAL;
							break;
						case Interface.OPER_STATE_TESTING:
							status = ObjectStatus.TESTING;
							break;
						case Interface.OPER_STATE_UP:
							status = ObjectStatus.NORMAL;
							break;
					}
					break;
			}
			gc.setBackground(StatusDisplayInfo.getStatusColor(status));
		}
      else if (displayMode == DISPLAY_MODE_STATUS)
      {
         gc.setBackground(StatusDisplayInfo.getStatusColor(p.getStatus()));
      }
      else if (p.isHighlighted())
      {
         gc.setBackground(colors.create(colorHighlight));
      }
		else
		{
         gc.setBackground(colors.create(colorBackground));
		}
      gc.setAlpha(127);
      gc.fillRectangle(rect);
      gc.setAlpha(255);
		gc.drawRectangle(rect);
		
      if (p.isHighlighted() && displayMode != DISPLAY_MODE_NONE)
      {
         Color c = gc.getForeground();
         int width = gc.getLineWidth();
         gc.setLineWidth(4);
         gc.setForeground(colors.create(colorHighlight));
         gc.drawRectangle(rect.x, rect.y, rect.width, rect.height);
         gc.setForeground(c);
         gc.setLineWidth(width);
      }
		
		if (selection == p)
		{
			gc.setLineStyle(SWT.LINE_DOT);
			gc.drawRectangle(rect.x + 2, rect.y + 2, rect.width - 4, rect.height - 4);
			gc.setLineStyle(SWT.LINE_SOLID);
		}

		Point ext = gc.textExtent(label);
      gc.drawText(label, point.x + (PORT_WIDTH - ext.x) / 2, point.y + (PORT_HEIGHT - ext.y) / 2, true);
	}

   /**
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
      int pic = 0;
      int maxX = 0;
      int maxY = 0;
      Point pos = null;
      PortCalculator portCalculator = createPortCalculator(nameSize.x);
      for(PortInfo p : ports)
      {
         if (p.getPIC() != pic)
         {
            pic = p.getPIC();
            if (pos != null)
               portCalculator = createPortCalculator(pos.x + PORT_WIDTH);
         }
         pos = portCalculator.calculateNextPos();
         if (pos.x > maxX)
            maxX = pos.x;
         if (pos.y > maxY)
            maxY = pos.y;
      }

      return new Point(maxX + PORT_WIDTH + HORIZONTAL_MARGIN, maxY + PORT_HEIGHT + VERTICAL_MARGIN);
	}

	/**
	 * @return the portStatusVisible
	 */
	public int getPortDisplayMode()
	{
		return displayMode;
	}

	/**
	 * @param portStatusVisible the portStatusVisible to set
	 */
	public void setPortDisplayMode(int displayMode)
	{
		this.displayMode = displayMode;
	}

	/**
	 * Clear port highlight
	 */
	void clearHighlight()
	{
		for(PortInfo pi : ports)
			pi.setHighlighted(false);
	}

	/**
	 * Add port highlight
	 * 
	 * @param p
	 */
	void addHighlight(Port p)
	{
		for(PortInfo pi : ports)
			if (pi.getPort() == p.getPort())
				pi.setHighlighted(true);
	}

   /**
    * @see org.eclipse.swt.events.MouseListener#mouseDoubleClick(org.eclipse.swt.events.MouseEvent)
    */
	@Override
	public void mouseDoubleClick(MouseEvent e)
	{
	}

   /**
    * @see org.eclipse.swt.events.MouseListener#mouseDown(org.eclipse.swt.events.MouseEvent)
    */
	@Override
	public void mouseDown(MouseEvent e)
	{
	}

   /**
    * @see org.eclipse.swt.events.MouseListener#mouseUp(org.eclipse.swt.events.MouseEvent)
    */
	@Override
	public void mouseUp(MouseEvent e)
	{
		PortInfo p = finder.findPortInfo(e.x, e.y);
		if (p != null && p != selection)
		{
			selection = p;
			redraw();
			for(PortSelectionListener listener : selectionListeners)
				listener.portSelected(p);
		}
	}

   /**
    * @see org.eclipse.swt.events.MouseTrackListener#mouseEnter(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseEnter(MouseEvent e)
   {
   }

   /**
    * @see org.eclipse.swt.events.MouseTrackListener#mouseExit(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseExit(MouseEvent e)
   {
      if ((tooltipDialog != null) && (tooltipDialog.getShell() != null) && !tooltipDialog.getShell().isDisposed() && (e.display.getActiveShell() != tooltipDialog.getShell()))
      {
         tooltipDialog.close();
         tooltipDialog = null;
      }
      tooltipObject = null;
   }

   /**
    * @see org.eclipse.swt.events.MouseTrackListener#mouseHover(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseHover(MouseEvent e)
   {
      PortInfo p = finder.findPortInfo(e.x, e.y);
      AbstractObject object = (p != null) ? Registry.getSession().findObjectById(p.getInterfaceObjectId()) : null;
      if ((object != null) && ((object != tooltipObject) || (tooltipDialog == null) || (tooltipDialog.getShell() == null) || tooltipDialog.getShell().isDisposed()))
      {
         if ((tooltipDialog != null) && (tooltipDialog.getShell() != null) && !tooltipDialog.getShell().isDisposed())
            tooltipDialog.close();
      
         tooltipObject = object;
         tooltipDialog = new ObjectPopupDialog(getShell(), object, toDisplay(e.x, e.y));
         tooltipDialog.open();
      }
      else if ((object == null) && (tooltipDialog != null) && (tooltipDialog.getShell() != null) && !tooltipDialog.getShell().isDisposed())
      {
         tooltipDialog.close();
         tooltipDialog = null;
      }
   }
	
	/**
	 * @return the selection
	 */
	public PortInfo getSelection()
	{
		return selection;
	}
	
	/**
	 * Add selection listener
	 * 
	 * @param listener
	 */
	public void addSelectionListener(PortSelectionListener listener)
	{
		selectionListeners.add(listener);
	}
	
	/**
	 * Remove selection listener
	 * 
	 * @param listener
	 */
	public void removeSelectionListener(PortSelectionListener listener)
	{
		selectionListeners.remove(listener);
	}

	@SuppressWarnings("rawtypes")
	private class PortLocationFinder
	{
	   private HashMap<Rectangle, PortInfo> portLocations = new HashMap<Rectangle, PortInfo>();
	   private List<Rectangle> sortedRectangles = new ArrayList<Rectangle>();
      private Comparator rectangleComparator;
	   
	   public PortLocationFinder()
	   {
	      rectangleComparator = new Comparator<Rectangle>() {
            @Override
            public int compare(Rectangle r1, Rectangle r2)
            {
               return r1.x - r2.x;
            }
	      };
	   }

	   /**
	    * Add port and its rectangle to the map
	    * @param rect
	    * @param port
	    */
      @SuppressWarnings("unchecked")
      private void addPortLocation(Rectangle rect, PortInfo port)
	   {
         portLocations.put(rect, port);
	      sortedRectangles.add(rect);
	      Collections.sort(sortedRectangles, rectangleComparator);
	   }
	   
	   /**
	    * Find PortInfo by coordinates
	    * @param x
	    * @param y
	    * @return PortInfo
	    */
	   private PortInfo findPortInfo(int x, int y)
	   {
	      for(Rectangle r : sortedRectangles)
	      {
	         if ((x >= r.x) && x < (r.x + PORT_WIDTH))
	         {
               if (r.contains(x, y))
                  return portLocations.get(r);
	         }
	      }
	         
	      return null;	            
	   }
	}
}
