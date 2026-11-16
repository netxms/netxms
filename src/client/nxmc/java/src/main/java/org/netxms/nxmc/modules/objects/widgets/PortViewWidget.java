/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import java.util.Map;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.Port;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.DashboardComposite;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortInfo;
import org.netxms.nxmc.modules.objects.widgets.helpers.PortSelectionListener;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.ComparatorHelper;
import org.netxms.nxmc.tools.FontTools;

/**
 * Widget for displaying switch/router ports
 */
public class PortViewWidget extends DashboardComposite
{
   // Marker colors for members of link aggregation interfaces
   private static final RGB[] AGGREGATE_COLORS = {
      new RGB(0, 114, 178), new RGB(230, 159, 0), new RGB(204, 121, 167), new RGB(86, 180, 233),
      new RGB(148, 103, 189), new RGB(240, 228, 66), new RGB(213, 94, 0), new RGB(0, 158, 115)
   };

	private long nodeId;
	private NXCSession session;
	private Map<Long, PortInfo> ports = new HashMap<Long, PortInfo>();
	private Map<Long, SlotViewWidget> slots = new HashMap<Long, SlotViewWidget>();
	private Label header = null;
	private Font headerFont = null;
	private int portDisplayMode = SlotViewWidget.DISPLAY_MODE_STATE;
   private int portScale = 100;
	private boolean headerVisible = false;
   private boolean aggregateLegendVisible = false;
   private Composite aggregateLegend = null;
   private ColorCache colors;
	private Set<PortSelectionListener> selectionListeners = new HashSet<PortSelectionListener>();
	private PortSelectionListener listener;

	/**
	 * @param parent
	 * @param style
	 */
	public PortViewWidget(Composite parent, int style)
	{
		super(parent, style);

      session = Registry.getSession();
      colors = new ColorCache(this);

		RowLayout layout = new RowLayout();
		layout.type = SWT.VERTICAL;
		layout.fill = true;
		layout.wrap = false;
		setLayout(layout);

		setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

		listener = new PortSelectionListener() {
			@Override
			public void portSelected(PortInfo port)
			{
				for(PortSelectionListener l : selectionListeners)
					l.portSelected(port);
			}
		};
	}

	/**
	 * Refresh widget
	 */
	public void refresh()
	{
		AbstractObject object = session.findObjectById(nodeId);
		if ((object == null) || !(object instanceof Node))
			return;

		if (!headerVisible && (header != null))
		{
		   header.dispose();
		   header = null;
		}
		for(SlotViewWidget s : slots.values())
		{
			s.setMenu(null);
			s.dispose();
		}
		slots.clear();
		ports.clear();
      if (aggregateLegend != null)
      {
         aggregateLegend.dispose();
         aggregateLegend = null;
      }

		if (headerVisible)
		{
		   if (header == null)
		   {
		      header = new Label(this, SWT.NONE);
		      header.setBackground(getBackground());
		      if (headerFont == null)
		      {
		         headerFont = FontTools.createTitleFont();
		         addDisposeListener(new DisposeListener() {
                  @Override
                  public void widgetDisposed(DisposeEvent e)
                  {
                     headerFont.dispose();
                  }
               });
		      }
		      header.setFont(headerFont);
		   }
		   header.setText(object.getObjectName());
		}

		List<Interface> interfaces = new ArrayList<Interface>();
		for(AbstractObject o: object.getAllChildren(AbstractObject.OBJECT_INTERFACE))
		{
         if (((Interface)o).isPhysicalPort() && ((Interface)o).isPhysicalLocationValid())
				interfaces.add((Interface)o);
		}
		Collections.sort(interfaces, new Comparator<Interface>() {
			@Override
			public int compare(Interface i1, Interface i2)
			{
				if (i1.getModule() == i2.getModule())
            {
               if (i1.getPIC() == i2.getPIC())
               {
                  if (i1.getPort() == i2.getPort())
                     return i1.getLane() - i2.getLane();
                  return i1.getPort() - i2.getPort();
               }
               return i1.getPIC() - i2.getPIC();
            }
				return i1.getModule() - i2.getModule();
			}
		});

		for(Interface iface : interfaces)
		{
			long hash = ((long)iface.getChassis() << 32) | (long)iface.getModule();
			SlotViewWidget sv = slots.get(hash);
			if (sv == null)
			{
				sv = new SlotViewWidget(this, SWT.NONE, String.format("%d/%d", iface.getChassis(), iface.getModule()), ((Node)object).getPortRowCount(), ((Node)object).getPortNumberingScheme());
				sv.setPortDisplayMode(portDisplayMode);
            sv.setScale(portScale);
				slots.put(hash, sv);
			}

			PortInfo p = new PortInfo(iface);
			ports.put(iface.getObjectId(), p);
			sv.addPort(p);
		}

      // Assign marker colors to link aggregation interfaces with member ports shown in this view
      List<Interface> aggregates = new ArrayList<Interface>();
      for(Interface iface : interfaces)
      {
         Interface aggregate = iface.getAggregateInterface();
         if ((aggregate != null) && !aggregates.contains(aggregate))
            aggregates.add(aggregate);
      }
      Collections.sort(aggregates, new Comparator<Interface>() {
         @Override
         public int compare(Interface i1, Interface i2)
         {
            return ComparatorHelper.compareStringsNatural(i1.getObjectName(), i2.getObjectName());
         }
      });
      Map<Long, RGB> aggregateColors = new HashMap<Long, RGB>();
      for(int i = 0; i < aggregates.size(); i++)
         aggregateColors.put(aggregates.get(i).getObjectId(), AGGREGATE_COLORS[i % AGGREGATE_COLORS.length]);
      for(SlotViewWidget sv : slots.values())
         sv.setAggregateColors(aggregateColors);

      if (aggregateLegendVisible && !aggregates.isEmpty())
         createAggregateLegend(aggregates, aggregateColors);

      layout(true, true);

		for(SlotViewWidget sv : slots.values())
		{
			sv.setMenu(getMenu());
			sv.addSelectionListener(listener);
		}
	}

   /**
    * Create legend for link aggregation markers.
    *
    * @param aggregates link aggregation interfaces
    * @param aggregateColors marker colors by object ID of link aggregation interface
    */
   private void createAggregateLegend(List<Interface> aggregates, Map<Long, RGB> aggregateColors)
   {
      aggregateLegend = new Composite(this, SWT.NONE);
      aggregateLegend.setBackground(getBackground());
      RowLayout layout = new RowLayout(SWT.HORIZONTAL);
      layout.wrap = true;
      layout.center = true;
      layout.spacing = 4;
      aggregateLegend.setLayout(layout);

      for(Interface aggregate : aggregates)
      {
         Label marker = new Label(aggregateLegend, SWT.BORDER);
         Color color = colors.create(aggregateColors.get(aggregate.getObjectId()));
         marker.setBackground(color);
         marker.setLayoutData(new RowData(12, 12));

         Label name = new Label(aggregateLegend, SWT.NONE);
         name.setBackground(getBackground());
         name.setText(aggregate.getObjectName() + "   ");
      }
   }

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @param nodeId the nodeId to set
	 */
	public void setNodeId(long nodeId)
	{
		this.nodeId = nodeId;
		refresh();
	}

	/**
	 * @return the portStatusVisible
	 */
	public int getPortDisplayMode()
	{
		return portDisplayMode;
	}

	/**
	 * @param portStatusVisible the portStatusVisible to set
	 */
	public void setPortDisplayMode(int portDisplayMode)
	{
		this.portDisplayMode = portDisplayMode;
	}
	
   /**
    * Get port size scale.
    *
    * @return scale in percents
    */
   public int getPortScale()
   {
      return portScale;
   }

   /**
    * Set port size scale. Takes effect on next refresh.
    *
    * @param portScale scale in percents (100 = default size)
    */
   public void setPortScale(int portScale)
   {
      this.portScale = portScale;
   }

	/**
    * @return the headerVisible
    */
   public boolean isHeaderVisible()
   {
      return headerVisible;
   }

   /**
    * @param headerVisible the headerVisible to set
    */
   public void setHeaderVisible(boolean headerVisible)
   {
      this.headerVisible = headerVisible;
   }

   /**
    * Check if legend for link aggregation markers is visible.
    *
    * @return true if legend for link aggregation markers is visible
    */
   public boolean isAggregateLegendVisible()
   {
      return aggregateLegendVisible;
   }

   /**
    * Show or hide legend for link aggregation markers. Takes effect on next refresh.
    *
    * @param aggregateLegendVisible true to show legend for link aggregation markers
    */
   public void setAggregateLegendVisible(boolean aggregateLegendVisible)
   {
      this.aggregateLegendVisible = aggregateLegendVisible;
   }

   /**
    * Set port highlight. Logical interfaces are highlighted as physical ports they are running over, and link aggregation
    * interfaces as their member ports.
    * 
    * @param ports ports to highlight
    */
	public void setHighlight(Port[] ports)
	{
		clearHighlight(false);

		for(Port p : ports)
		{
		   Interface iface = session.findObjectById(p.getObjectId(), Interface.class);
		   if (iface != null)
		   {
		      for(Interface physicalPort : iface.getPhysicalPorts())
		         addHighlight(new Port(physicalPort));
		   }
		   else if (p.isLocationValid())
		   {
		      addHighlight(p);
		   }
		}

		for(SlotViewWidget sv : slots.values())
			sv.redraw();
	}
	
   /**
    * Add highlight for port with known location.
    *
    * @param p port to highlight
    */
   private void addHighlight(Port p)
   {
      SlotViewWidget sv = slots.get(((long)p.getChassis() << 32) | (long)p.getModule());
      if (sv != null)
         sv.addHighlight(p);
   }

	/**
	 * Clear port highlight.
	 * 
	 * @param doRedraw if true, control will be redrawn
	 */
	public void clearHighlight(boolean doRedraw)
	{
		for(SlotViewWidget sv : slots.values())
		{
			sv.clearHighlight();
			if (doRedraw)
				sv.redraw();
		}
	}

   /**
    * @see org.eclipse.swt.widgets.Control#setMenu(org.eclipse.swt.widgets.Menu)
    */
	@Override
	public void setMenu(Menu menu)
	{
		super.setMenu(menu);
		for(SlotViewWidget sv : slots.values())
			sv.setMenu(getMenu());
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
