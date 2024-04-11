/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.tools.widgets.helpers;

import java.net.InetAddress;
import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ConnectionPointType;
import org.netxms.client.objects.Interface;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.tools.widgets.SearchResult;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.ViewerElementUpdater;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for connection point objects
 *
 */
public class ConnectionPointLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(ConnectionPointLabelProvider.class);
   
	private static final Color COLOR_FOUND_OBJECT_DIRECT = new Color(Display.getDefault(), ColorConverter.parseColorDefinition("DarkGreen"));
	private static final Color COLOR_FOUND_OBJECT_INDIRECT = new Color(Display.getDefault(), ColorConverter.parseColorDefinition("SeaGreen"));
   private static final Color COLOR_FOUND_OBJECT_WIRELESS = new Color(Display.getDefault(), ColorConverter.parseColorDefinition("Teal"));
   private static final Color COLOR_FOUND_OBJECT_UNKNOWN = new Color(Display.getDefault(), ColorConverter.parseColorDefinition("Peru"));
	private static final Color COLOR_FOUND_MAC_DIRECT = new Color(Display.getDefault(), ColorConverter.parseColorDefinition("DarkBlue"));
	private static final Color COLOR_FOUND_MAC_INDIRECT = new Color(Display.getDefault(), ColorConverter.parseColorDefinition("DarkSlateBlue"));
   private static final Color COLOR_FOUND_MAC_WIRELESS = new Color(Display.getDefault(), ColorConverter.parseColorDefinition("SteelBlue"));
	private static final Color COLOR_NOT_FOUND = new Color(Display.getDefault(), ColorConverter.parseColorDefinition("DarkRed"));
   private static final Color COLOR_HISTORICAL = ThemeEngine.getForegroundColor("List.DisabledItem");

	private Map<Long, String> cachedObjectNames = new HashMap<Long, String>();
   private NXCSession session = Registry.getSession();
   private TableViewer viewer;

	/**
	 * Constructor
	 */
	public ConnectionPointLabelProvider(TableViewer viewer)
	{
	   this.viewer = viewer;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	/**
	 * Get (cached) object name by object ID
	 * @param id object id
	 * @return object name
	 */
	private String getObjectName(long id)
	{
		if (id == 0)
			return ""; //$NON-NLS-1$

		String name = cachedObjectNames.get(id);
		if (name == null)
		{
			name = session.getObjectName(id);
			cachedObjectNames.put(id, name);
		}
		return name;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		ConnectionPoint cp = (ConnectionPoint)element;
		switch(columnIndex)
		{
			case SearchResult.COLUMN_SEQUENCE:
				return Integer.toString((Integer)cp.getData() + 1);
			case SearchResult.COLUMN_NODE:
				return getObjectName(cp.getLocalNodeId());
			case SearchResult.COLUMN_INTERFACE:
				return getObjectName(cp.getLocalInterfaceId());
			case SearchResult.COLUMN_MAC_ADDRESS:
            return (cp.getLocalMacAddress() != null) ? cp.getLocalMacAddress().toString() : "n/a";
         case SearchResult.COLUMN_NIC_VENDOR:
            if (cp.getLocalMacAddress() == null)
               return "";
            return session.getVendorByMac(cp.getLocalMacAddress(), new ViewerElementUpdater(viewer, element));
			case SearchResult.COLUMN_IP_ADDRESS:
				InetAddress addr = cp.getLocalIpAddress();
				if (addr != null)
					return addr.getHostAddress();
				Interface iface = (Interface)session.findObjectById(cp.getLocalInterfaceId(), Interface.class);
				if (iface == null)
				   return ""; //$NON-NLS-1$
				InetAddress a = iface.getFirstUnicastAddress();
				return (a != null) ? a.getHostAddress() : ""; //$NON-NLS-1$
			case SearchResult.COLUMN_SWITCH:
            return (cp.getNodeId() != 0) ? getObjectName(cp.getNodeId()) : "n/a";
			case SearchResult.COLUMN_PORT:
            return (cp.getInterfaceId() != 0) ? getObjectName(cp.getInterfaceId()) : "n/a";
         case SearchResult.COLUMN_INDEX:
            return Integer.toString(cp.getInterfaceIndex());
			case SearchResult.COLUMN_TYPE:
			   if (cp.getType() == null)
               return "n/a";
			   switch(cp.getType())
			   {
			      case DIRECT:
			         return i18n.tr("direct");
               case INDIRECT:
                  return i18n.tr("indirect");
               case WIRELESS:
                  return i18n.tr("wireless");
               case NOT_FOUND:
                  return i18n.tr("not found");
               default:
                  return i18n.tr("unknown");
			   }
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
	@Override
	public Color getForeground(Object element)
	{
		ConnectionPoint cp = (ConnectionPoint)element;
		if (cp.getType() == ConnectionPointType.NOT_FOUND)
		   return COLOR_NOT_FOUND;
		
		if (cp.getLocalNodeId() == 0)
		   return (cp.getType() == ConnectionPointType.DIRECT) ? COLOR_FOUND_MAC_DIRECT : 
		         ((cp.getType() == ConnectionPointType.WIRELESS) ? COLOR_FOUND_MAC_WIRELESS : COLOR_FOUND_MAC_INDIRECT);
		    
		          
      switch(cp.getType())
      {
         case DIRECT:
            return COLOR_FOUND_OBJECT_DIRECT;
         case INDIRECT:
            return COLOR_FOUND_OBJECT_INDIRECT;
         case WIRELESS:
            return COLOR_FOUND_OBJECT_WIRELESS;
         case UNKNOWN:
            return COLOR_FOUND_OBJECT_UNKNOWN;
         case NOT_FOUND:
            return COLOR_NOT_FOUND;
      }

		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
	@Override
	public Color getBackground(Object element)
	{
      ConnectionPoint cp = (ConnectionPoint)element;
      if (cp.isHistorical())
         return COLOR_HISTORICAL;
      return null;
	}
}
