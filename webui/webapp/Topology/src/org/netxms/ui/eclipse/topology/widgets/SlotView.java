/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Raden Solutions
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
package org.netxms.ui.eclipse.topology.widgets;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.Interface;
import org.netxms.client.topology.Port;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortCalculator;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortCalculatorDownUpLeftRight;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortCalculatorLeftRightDownUp;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortCalculatorLeftRightUpDown;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortCalculatorUpDownLeftRight;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortInfo;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortSelectionListener;

/**
 * Single slot view
 */
public class SlotView extends Canvas implements PaintListener, MouseListener
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
	private static final int HORIZONTAL_SPACING = 10;
	private static final int VERTICAL_SPACING = 10;
	private static final int PORT_WIDTH = 44;
	private static final int PORT_HEIGHT = 30;
	
	private static final RGB BACKGROUND_COLOR = new RGB(224, 224, 224);
	private static final RGB HIGHLIGHT_COLOR = new RGB(64, 156, 224);
	
	private List<PortInfo> ports = new ArrayList<PortInfo>();
	private int rowCount;
	private int numberingScheme;
	private String slotName;
	private Point nameSize;
	private boolean portStatusVisible = true;
	private PortInfo selection = null;
	private Set<PortSelectionListener> selectionListeners = new HashSet<PortSelectionListener>();
	private ColorCache colors;
	
	/**
	 * @param parent
	 * @param style
	 */
	public SlotView(Composite parent, int style, String slotName, int rowCount, int numberingScheme)
	{
		super(parent, style | SWT.BORDER);
		this.slotName = slotName;
		this.numberingScheme = numberingScheme;
		this.rowCount = rowCount;
		
		colors = new ColorCache(this);
		
		GC gc = new GC(getDisplay());
		nameSize = gc.textExtent(slotName);
		gc.dispose();
		
		addPaintListener(this);
		addMouseListener(this);
	}
	
	/**
	 * @param p
	 */
	public void addPort(PortInfo p)
	{
		ports.add(p);
	}
	
	/**
	 * Get port info object from given point
	 * 
	 * @param x
	 * @param y
	 * @return
	 */
	private PortInfo getPortFromPoint(int x, int y)
	{
		final int xOffset = HORIZONTAL_MARGIN + nameSize.x + HORIZONTAL_SPACING;
		
		if ((x < xOffset) || (y < VERTICAL_MARGIN))
			return null;	// x before first column
		
		int column = (x - xOffset) / (PORT_WIDTH + HORIZONTAL_SPACING);
		if (column >= (ports.size() + rowCount - 1) / rowCount)
			return null;	// x after last column
		
		if (x > xOffset + (column * (PORT_WIDTH + HORIZONTAL_SPACING)) + PORT_WIDTH)
			return null;	// x inside spacing after column
		
		int row = (y - VERTICAL_MARGIN) / (PORT_HEIGHT + VERTICAL_SPACING);
		if (row >= rowCount)
			return null;	// y after last row
		
		if (y > VERTICAL_MARGIN + (row * (PORT_HEIGHT + VERTICAL_SPACING)) + PORT_HEIGHT)
			return null;	// y inside spacing after row
		
		return ports.get(column * rowCount + row);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		e.gc.drawText(slotName, HORIZONTAL_MARGIN, (getSize().y - nameSize.y) / 2);
		
		PortCalculator portCalculator = null;
		switch(numberingScheme)
		{
		   case NDD_PN_DU_LR:
		      portCalculator = new PortCalculatorDownUpLeftRight(nameSize.x, rowCount);
		      break;
		   case NDD_PN_LR_UD:
		      portCalculator = new PortCalculatorLeftRightUpDown(nameSize.x, ports.size(), rowCount);
		      break;
		   case NDD_PN_LR_DU:
		      portCalculator = new PortCalculatorLeftRightDownUp(nameSize.x, ports.size(), rowCount);
		      break;
		   case NDD_PN_CUSTOM:
		      break;
         case NDD_PN_UNKNOWN:
         case NDD_PN_UD_LR:
		   default:
            portCalculator = new PortCalculatorUpDownLeftRight(nameSize.x, rowCount);
            break;
		}
		
		for(PortInfo p : ports)
		{
			drawPort(p, portCalculator.getXPos(), portCalculator.getYPos(), e.gc);
         portCalculator.calculateNextPos();
		}
	}
	
	/**
	 * Draw single port
	 * 
	 * @param p port information
	 * @param x X coordinate of top left corner
	 * @param y Y coordinate of top left corner
	 */
	private void drawPort(PortInfo p, int x, int y, GC gc)
	{
		final String label = Integer.toString(p.getPort());
		Rectangle rect = new Rectangle(x, y, PORT_WIDTH, PORT_HEIGHT);
		
		if (p.isHighlighted())
		{
			gc.setBackground(colors.create(HIGHLIGHT_COLOR));
			gc.fillRectangle(rect);
		}
		else if (portStatusVisible)
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
			gc.fillRectangle(rect);
		}
		else
		{
			gc.setBackground(colors.create(BACKGROUND_COLOR));
			gc.fillRectangle(rect);
		}
		gc.drawRectangle(rect);
		
		if (selection == p)
		{
			//gc.setLineStyle(SWT.LINE_DOT);
			gc.drawRectangle(rect.x + 2, rect.y + 2, rect.width - 4, rect.height - 4);
			//gc.setLineStyle(SWT.LINE_SOLID);
		}
		
		Point ext = gc.textExtent(label);
		gc.drawText(label, x + (PORT_WIDTH - ext.x) / 2, y + (PORT_HEIGHT - ext.y) / 2);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		return new Point(((ports.size() + rowCount - 1) / rowCount) * (PORT_WIDTH + HORIZONTAL_SPACING) + HORIZONTAL_MARGIN * 2 + nameSize.x,
				rowCount * PORT_HEIGHT + (rowCount - 1) * VERTICAL_SPACING + VERTICAL_MARGIN * 2);
	}
	
	/**
	 * @return the portStatusVisible
	 */
	public boolean isPortStatusVisible()
	{
		return portStatusVisible;
	}

	/**
	 * @param portStatusVisible the portStatusVisible to set
	 */
	public void setPortStatusVisible(boolean portStatusVisible)
	{
		this.portStatusVisible = portStatusVisible;
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

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.MouseListener#mouseDoubleClick(org.eclipse.swt.events.MouseEvent)
	 */
	@Override
	public void mouseDoubleClick(MouseEvent e)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.MouseListener#mouseDown(org.eclipse.swt.events.MouseEvent)
	 */
	@Override
	public void mouseDown(MouseEvent e)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.MouseListener#mouseUp(org.eclipse.swt.events.MouseEvent)
	 */
	@Override
	public void mouseUp(MouseEvent e)
	{
		PortInfo p = getPortFromPoint(e.x, e.y);
		if (p != selection)
		{
			selection = p;
			redraw();
			for(PortSelectionListener listener : selectionListeners)
				listener.portSelected(p);
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
}
