/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.base.InetAddressEx;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.client.objects.interfaces.HardwareEntity;
import org.netxms.nxmc.base.widgets.RoundedLabel;
import org.netxms.nxmc.base.widgets.Section;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Information card for a selected rack element, shared by the 2D and 3D rack
 * views. Shows the element name as the card title, a rounded status label, and
 * overview-style detail rows (rows with empty values are omitted). The card
 * hides itself when nothing is selected. The hosting layout is expected to give
 * it a fixed width so the rack drawing area does not change size when the
 * selection changes.
 */
public class RackElementInfoCard extends Section
{
   private final I18n i18n = LocalizationHelper.getI18n(RackElementInfoCard.class);

   /**
    * @param parent parent composite
    */
   public RackElementInfoCard(Composite parent)
   {
      super(parent, "", false);
      GridLayout layout = new GridLayout(2, false);
      layout.marginWidth = 6;
      layout.marginHeight = 6;
      getClient().setLayout(layout);
      setVisible(false);
   }

   /**
    * Update the card for the given selection. Accepts a {@link HardwareEntity}
    * (node, chassis) or a {@link PassiveRackElement}; null hides the card.
    *
    * @param selection selected rack element or null
    */
   public void update(Object selection)
   {
      Composite client = getClient();
      for(Control c : client.getChildren())
         c.dispose();

      if (selection instanceof HardwareEntity)
      {
         HardwareEntity e = (HardwareEntity)selection;
         setText(e.getObjectName());

         RoundedLabel status = new RoundedLabel(client);
         GridData gd = new GridData(SWT.LEFT, SWT.CENTER, false, false);
         gd.horizontalSpan = 2;
         status.setLayoutData(gd);
         status.setText(StatusDisplayInfo.getStatusText(e.getStatus()));
         status.setLabelBackground(StatusDisplayInfo.getStatusBackgroundColor(e.getStatus()));

         if (e instanceof AbstractNode)
         {
            AbstractNode n = (AbstractNode)e;
            InetAddressEx ip = n.getPrimaryIP();
            if ((ip != null) && ip.isValidUnicastAddress())
               addRow(client, i18n.tr("Primary IP"), ip.getHostAddress());
            addRow(client, i18n.tr("Vendor"), n.getHardwareVendor());
            addRow(client, i18n.tr("Model"), n.getHardwareProductName());
            addRow(client, i18n.tr("Version"), n.getHardwareProductVersion());
            addRow(client, i18n.tr("Serial number"), n.getHardwareSerialNumber());
            addRow(client, i18n.tr("Location"), n.getSnmpSysLocation());
            addRow(client, i18n.tr("Contact"), n.getSnmpSysContact());
         }

         if (e.getRackPosition() > 0)
         {
            addRow(client, i18n.tr("Position"), Integer.toString(e.getRackPosition()));
            addRow(client, i18n.tr("Height"), e.getRackHeight() + "U");
            if (e.getRackOrientation() != null)
               addRow(client, i18n.tr("Side"), e.getRackOrientation().toString());
         }
         setVisible(true);
      }
      else if (selection instanceof PassiveRackElement)
      {
         PassiveRackElement p = (PassiveRackElement)selection;
         String name = p.getName();
         setText(((name != null) && !name.isEmpty()) ? name
               : ((p.getType() != null) ? p.getType().toString() : i18n.tr("Passive element")));
         if (p.getType() != null)
            addRow(client, i18n.tr("Type"), p.getType().toString());
         addRow(client, i18n.tr("Position"), Integer.toString(p.getPosition()));
         addRow(client, i18n.tr("Height"), p.getHeight() + "U");
         if (p.getOrientation() != null)
            addRow(client, i18n.tr("Side"), p.getOrientation().toString());
         setVisible(true);
      }
      else
      {
         setVisible(false);
         return;
      }

      client.layout();
      redraw(); // Card.setText() updates the title field but does not repaint the header
      getParent().layout();
   }

   /**
    * Add a caption/value row, skipping it entirely when the value is empty.
    */
   private static void addRow(Composite parent, String caption, String value)
   {
      if ((value == null) || value.isEmpty())
         return;
      Label c = new Label(parent, SWT.NONE);
      c.setText(caption + ":");
      c.setBackground(parent.getBackground());
      Label v = new Label(parent, SWT.NONE);
      v.setText(value);
      v.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
      v.setBackground(parent.getBackground());
   }
}
