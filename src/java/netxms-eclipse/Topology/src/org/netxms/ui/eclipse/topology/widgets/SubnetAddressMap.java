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
package org.netxms.ui.eclipse.topology.widgets;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.Subnet;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.topology.Activator;

/**
 * Subnet address map
 */
public class SubnetAddressMap extends Canvas implements PaintListener, MouseTrackListener
{
	private static final int HORIZONTAL_MARGIN = 20;
	private static final int VERTICAL_MARGIN = 10;
	private static final int HORIZONTAL_SPACING = 10;
	private static final int VERTICAL_SPACING = 10;
	private static final int ELEMENT_WIDTH = 70;
	private static final int ELEMENT_HEIGHT = 25;
   private static final int ELEMENTS_PER_ROW = 16;
   private static final int MAX_ROWS = 256;
	
	private static final RGB COLOR_RESERVED = new RGB(224, 224, 224);
   private static final RGB COLOR_USED = new RGB(237, 24, 35);
   private static final RGB COLOR_FREE = new RGB(0, 220, 55);

	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	private IViewPart viewPart;
	private long[] addressMap = null;
	private Subnet subnet = null;
	private int rowCount = 0;
	private ColorCache colors;
	
	/**
	 * @param parent
	 * @param style
	 */
	public SubnetAddressMap(Composite parent, int style, IViewPart viewPart)
	{
		super(parent, style | SWT.DOUBLE_BUFFERED);

		this.viewPart = viewPart;
		colors = new ColorCache(this);
		
		addPaintListener(this);
		addMouseTrackListener(this);
	}
	
	/**
	 * @param subnet
	 */
	public void setSubnet(Subnet subnet)
	{
	   this.subnet = subnet;
      rowCount = ((1 << (32 - subnet.getMaskBits())) + ELEMENTS_PER_ROW - 1) / ELEMENTS_PER_ROW;
      if (rowCount > MAX_ROWS)
         rowCount = MAX_ROWS;
	   refresh();
	}
	
	/**
	 * Refresh widget
	 */
	public void refresh()
	{
	   if (subnet == null)
	   {
	      addressMap = null;
	      rowCount = 0;
	      redraw();
         return;
	   }
	   
	   new ConsoleJob("Get subnet address map", viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final long[] map = session.getSubnetAddressMap(subnet.getObjectId());
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     addressMap = map;
                     rowCount = (map.length + ELEMENTS_PER_ROW - 1) / ELEMENTS_PER_ROW;
                     if (rowCount > MAX_ROWS)
                        rowCount = MAX_ROWS;
                     redraw();
                  }
               });
            }
            catch(NXCException e)
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     addressMap = null;
                     rowCount = 0;
                     redraw();
                  }
               });
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot get address map for subnet";
         }
      }.start();
	}
	
	/**
	 * Get address from given point
	 * 
	 * @param x
	 * @param y
	 * @return
	 */
	private int getElementFromPoint(int x, int y)
	{
		final int xOffset = HORIZONTAL_MARGIN;
		
		if ((x < xOffset) || (y < VERTICAL_MARGIN))
			return -1;	// x before first column
		
		int column = (x - xOffset) / (ELEMENT_WIDTH + HORIZONTAL_SPACING);
		if (column >= ELEMENTS_PER_ROW)
			return -1;	// x after last column
		
		if (x > xOffset + (column * (ELEMENT_WIDTH + HORIZONTAL_SPACING)) + ELEMENT_WIDTH)
			return -1;	// x inside spacing after column
		
		int row = (y - VERTICAL_MARGIN) / (ELEMENT_HEIGHT + VERTICAL_SPACING);
		if (row >= rowCount)
			return -1;	// y after last row
		
		if (y > VERTICAL_MARGIN + (row * (ELEMENT_HEIGHT + VERTICAL_SPACING)) + ELEMENT_HEIGHT)
			return -1;	// y inside spacing after row
		
		return row * ELEMENTS_PER_ROW + column;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
	   if (addressMap == null)
	      return;
	   
		int x = HORIZONTAL_MARGIN;
		int y = VERTICAL_MARGIN;
		int row = 0;
		int column = 0;
      byte[] address = subnet.getPrimaryIP().getAddress();
		for(int i = 0; i < addressMap.length; i++)
		{
			drawAddress(address, addressMap[i], x, y, e.gc);
			column++;
			if (column == ELEMENTS_PER_ROW)
			{
			   row++;
			   if (row >= MAX_ROWS)
			      break;
				column = 0;
				x = HORIZONTAL_MARGIN;
				y += VERTICAL_SPACING + ELEMENT_HEIGHT;
			}
			else
			{
				x += HORIZONTAL_SPACING + ELEMENT_WIDTH;
			}
         if (address[address.length - 1] != (byte)0xFF)
         {
            address[address.length - 1]++;
         }
         else
         {
            address[address.length - 2]++;
            address[address.length - 1] = 0;
         }
		}
	}
	
	/**
	 * Draw single address
	 * 
	 * @param a IP address
	 * @param x X coordinate of top left corner
	 * @param y Y coordinate of top left corner
	 */
	private void drawAddress(byte[] address, long nodeId, int x, int y, GC gc)
	{
		final String label = (subnet.getMaskBits() < 24) ? 
		      ("." + ((int)address[address.length - 2] & 0xFF) + "." + ((int)address[address.length - 1] & 0xFF)) :
		         ("." + ((int)address[address.length - 1] & 0xFF));
		Rectangle rect = new Rectangle(x, y, ELEMENT_WIDTH, ELEMENT_HEIGHT);
		
		if (nodeId == 0)
		{
         gc.setBackground(colors.create(COLOR_FREE));
		}
		else if (nodeId == 0xFFFFFFFFL)
      {
         gc.setBackground(colors.create(COLOR_RESERVED));
      }
		else
		{
		   gc.setBackground(colors.create(COLOR_USED));
		}
		gc.fillRectangle(rect);
		gc.drawRectangle(rect);
		
		Point ext = gc.textExtent(label);
		gc.drawText(label, x + (ELEMENT_WIDTH - ext.x) / 2, y + (ELEMENT_HEIGHT - ext.y) / 2);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		return new Point(ELEMENTS_PER_ROW * ELEMENT_WIDTH + (ELEMENTS_PER_ROW - 1) * HORIZONTAL_SPACING + HORIZONTAL_MARGIN * 2,
				rowCount * ELEMENT_HEIGHT + (rowCount - 1) * VERTICAL_SPACING + VERTICAL_MARGIN * 2);
	}

   /* (non-Javadoc)
    * @see org.eclipse.swt.events.MouseTrackListener#mouseEnter(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseEnter(MouseEvent e)
   {
   }

   /* (non-Javadoc)
    * @see org.eclipse.swt.events.MouseTrackListener#mouseExit(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseExit(MouseEvent e)
   {
   }

   /* (non-Javadoc)
    * @see org.eclipse.swt.events.MouseTrackListener#mouseHover(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseHover(MouseEvent e)
   {
      int index = getElementFromPoint(e.x, e.y);
      if (index == -1)
      {
         setToolTipText(null);
      }
      else if (index == 0)
      {
         setToolTipText("Subnet address");
      }
      else if (index == addressMap.length - 1)
      {
         setToolTipText("Broadcast address");
      }
      else
      {
         if (addressMap[index] == 0)
         {
            setToolTipText("Free");
         }
         else
         {
            AbstractNode node = (AbstractNode)session.findObjectById(addressMap[index], AbstractNode.class);
            if (node != null)
            {
               setToolTipText(node.getObjectName());
            }
            else
            {
               setToolTipText("[" + addressMap[index] + "]");
            }
         }
      }
   }
}
