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

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ZoneSelectionDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Zone selector
 */
public class ZoneSelector extends AbstractSelector
{
   private I18n i18n = LocalizationHelper.getI18n(ZoneSelector.class);
   private int zoneUIN = -1;
   private String emptySelectionName = i18n.tr("<none>");

   /**
    * Create zone selector.
    *
    * @param parent parent composite
    * @param style widget style
    * @param configurator selector configurator
    */
   public ZoneSelector(Composite parent, int style, SelectorConfigurator configurator)
   {
      super(parent, style, configurator);
      setText(emptySelectionName);
   }

   /**
    * Create zone selector.
    *
    * @param parent parent composite
    * @param style widget style
    * @param showClearButton true to show clear button
    */
   public ZoneSelector(Composite parent, int style, boolean showClearButton)
   {
      this(parent, style, new SelectorConfigurator().setShowClearButton(showClearButton));
   }

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractSelector#selectionButtonHandler()
    */
   @Override
   protected void selectionButtonHandler()
   {     
      ZoneSelectionDialog dlg = new ZoneSelectionDialog(getShell());
      if (dlg.open() == Window.OK)
      {
         zoneUIN = dlg.getZoneUIN();
         setText(dlg.getZoneName());
         fireModifyListeners();
      }
   }

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractSelector#clearButtonHandler()
    */
   @Override
   protected void clearButtonHandler()
   {
      zoneUIN = -1;
      setText(emptySelectionName);
      fireModifyListeners();
   }

   /**
    * Get UIN of selected zone
    * 
    * @return selected zone UIN
    */
   public int getZoneUIN()
   {
      return zoneUIN;
   }

   /**
    * Get name of selected zone
    * 
    * @return zone name
    */
   public String getZoneName()
   {
      return getText();
   }

   /**
    * Set zone UIN
    * 
    * @param zoneUIN new zone UIN
    */
   public void setZoneUIN(int zoneUIN)
   {
      this.zoneUIN = zoneUIN;
      if (zoneUIN == -1)
      {
         setText(emptySelectionName);
      }
      else
      {
         final Zone zone = Registry.getSession().findZone(zoneUIN);
         setText((zone != null) ? zone.getObjectName() : ("<" + Long.toString(zoneUIN) + ">"));
      }
   }

   /**
    * Set empty selection text
    * @param text to set
    */
   public void setEmptySelectionText(String text)
   {
      emptySelectionName = text;
      setText(emptySelectionName);
   }
}
