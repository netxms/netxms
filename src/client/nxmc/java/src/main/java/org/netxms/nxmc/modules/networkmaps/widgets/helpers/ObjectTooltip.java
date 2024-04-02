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
package org.netxms.nxmc.modules.networkmaps.widgets.helpers;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.GridData;
import org.eclipse.draw2d.GridLayout;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.MarginBorder;
import org.eclipse.draw2d.text.FlowPage;
import org.eclipse.draw2d.text.TextFlow;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.netxms.base.MacAddress;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.maps.MapObjectDisplayMode;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.UnknownObject;
import org.netxms.client.topology.RadioInterface;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Tooltip for object on map
 */
public class ObjectTooltip extends Figure
{
   private static final Logger logger = LoggerFactory.getLogger(ObjectTooltip.class);

   private final I18n i18n = LocalizationHelper.getI18n(ObjectTooltip.class);

   private NXCSession session = Registry.getSession();
   private NodeLastValuesFigure lastValuesFigure = null;
   private int index;
   private AbstractObject object;
   private MapLabelProvider labelProvider;
   private long refreshTimestamp = 0;
   private long mapId = 0;

	/**
	 * @param object
	 */
   public ObjectTooltip(NetworkMapObject element, final long mapId, MapLabelProvider labelProvider)
	{
      this.labelProvider = labelProvider;
      this.mapId = mapId;

      object = session.findObjectById(element.getObjectId(), true);
      if (object == null)
         object = new UnknownObject(element.getObjectId(), session);

      setBorder(new MarginBorder(3));
		GridLayout layout = new GridLayout(2, false);
		layout.horizontalSpacing = 10;
		setLayoutManager(layout);

      setOpaque(true);
      setBackgroundColor(ThemeEngine.getBackgroundColor("Map.ObjectTooltip"));

      Label title = new Label();
		title.setIcon(labelProvider.getSmallIcon(object));
		title.setText(object.getObjectName());
		title.setFont(JFaceResources.getBannerFont());
		add(title);

		Label status = new Label();
		status.setIcon(StatusDisplayInfo.getStatusImage(object.getStatus()));
		status.setText(StatusDisplayInfo.getStatusText(object.getStatus()));
		add(status);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.RIGHT;
		setConstraint(status, gd);

      if (object instanceof Node)
      {
         if ((((Node)object).getCapabilities() & AbstractNode.NC_IS_ETHERNET_IP) != 0)
         {
            if ((((Node)object).getHardwareProductName() != null) && !((Node)object).getHardwareProductName().isEmpty())
               addInformationBlock(((Node)object).getHardwareProductName() + " (" + ((Node)object).getCipDeviceTypeName() + ")", null);
            else
               addInformationBlock(((Node)object).getCipDeviceTypeName(), null);
         }
         else
         {
            addInformationBlock(((Node)object).getHardwareProductName(), null);
         }
         addInformationBlock(((Node)object).getHardwareVendor(), "Vendor: ");
      }

		if ((object instanceof Node) && ((Node)object).getPrimaryIP().isValidAddress() && !((Node)object).getPrimaryIP().getAddress().isAnyLocalAddress())
		{
			StringBuilder sb = new StringBuilder(((Node)object).getPrimaryIP().getHostAddress());
			MacAddress mac = ((Node)object).getPrimaryMAC();
			if (mac != null)
			{
				sb.append(" ("); //$NON-NLS-1$
				sb.append(mac.toString());
				sb.append(')');
			}

			Label iface = new Label();
			iface.setIcon(SharedIcons.IMG_IP_ADDRESS);
			iface.setText(sb.toString());
			add(iface);

			gd = new GridData();
			gd.horizontalSpan = 2;
			setConstraint(iface, gd);
		}

      index = getChildren().size();		
		if (object instanceof Node && labelProvider.getObjectFigureType() == MapObjectDisplayMode.LARGE_LABEL)
		{
   		DciValue[] values = labelProvider.getNodeLastValues(object.getObjectId());
   		if ((values != null) && (values.length > 0))
   		{
      		lastValuesFigure = new NodeLastValuesFigure(values);
            add(lastValuesFigure);
            
             gd = new GridData();
            gd.horizontalSpan = 2;
            setConstraint(lastValuesFigure, gd);
         }
      }

		if (object instanceof AccessPoint)
		{
			StringBuilder sb = new StringBuilder(((AccessPoint)object).getModel());
			MacAddress mac = ((AccessPoint)object).getMacAddress();
			if (mac != null)
			{
				sb.append('\n');
				sb.append(mac.toString());
			}

			for(RadioInterface rif : ((AccessPoint)object).getRadios())
			{
            sb.append(i18n.tr("\nRadio"));
				sb.append(rif.getIndex());
            sb.append(" (");
            sb.append(rif.getBSSID().toString());
            sb.append(i18n.tr(")\n\tChannel: "));
				sb.append(rif.getChannel());
            sb.append(i18n.tr("\n\tTX power: "));
				sb.append(rif.getPowerMW());
            sb.append(i18n.tr(" mW"));
			}

			Label info = new Label();
			info.setText(sb.toString());
			add(info);

			gd = new GridData();
			gd.horizontalSpan = 2;
			setConstraint(info, gd);
		}

		if (!object.getComments().isEmpty())
		{
			FlowPage page = new FlowPage();
			add(page);
			gd = new GridData();
			gd.horizontalSpan = 2;
			setConstraint(page, gd);

			TextFlow text = new TextFlow();
			text.setText("\n" + object.getComments()); //$NON-NLS-1$
			page.add(text);
		}
	}

   /**
    * Refresh tooltip
    */
   public void refresh()
   {
      long now = System.currentTimeMillis();
      if (now < refreshTimestamp + 15000)
         return;

      refreshTimestamp = now;
	   if (labelProvider.getObjectFigureType() == MapObjectDisplayMode.LARGE_LABEL || !(object instanceof DataCollectionTarget))
	      return;
	   
      final long nodeId = object.getObjectId();
      Job job = new Job("Get DCI data for object tooltip", null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final DciValue[] values = session.getDataCollectionSummary(nodeId, mapId, true, false, false);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if ((object instanceof Node) && (values != null) && (values.length > 0))
                     {
                        if (lastValuesFigure != null)
                           remove(lastValuesFigure);
                        lastValuesFigure = new NodeLastValuesFigure(values);
                        GridData gd = new GridData();
                        gd.horizontalSpan = 2;
                        add(lastValuesFigure, gd, index);
                        layout();
                        labelProvider.getViewer().resizeToolTipShell();
                     }
                  }
               });
            }
            catch(Exception e)
            {
               logger.error("Exception while reading data for object tooltip", e);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return null;
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
	}

   /**
    * Add additional information block
    * 
    * @param text
    */
   private void addInformationBlock(String text, String prefix)
   {
      if ((text == null) || text.isEmpty())
         return;

      Label info = new Label();
      info.setText((prefix != null) ? (prefix + text) : text);
      add(info);

      GridData gd = new GridData();
      gd.horizontalSpan = 2;
      setConstraint(info, gd);
   }
}
