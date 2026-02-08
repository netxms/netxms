/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.events.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.dialogs.MultiEventSelectionDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Multi-event selector widget. Allows selection of multiple events
 * and displays them as a comma-separated list.
 */
public class MultiEventSelector extends AbstractSelector
{
   private final I18n i18n = LocalizationHelper.getI18n(MultiEventSelector.class);

   private int[] eventCodes = new int[0];

   /**
    * Create multi-event selector.
    *
    * @param parent parent composite
    * @param style widget style
    */
   public MultiEventSelector(Composite parent, int style)
   {
      this(parent, style, new SelectorConfigurator());
   }

   /**
    * Create multi-event selector with custom configurator.
    *
    * @param parent parent composite
    * @param style widget style
    * @param configurator selector configurator
    */
   public MultiEventSelector(Composite parent, int style, SelectorConfigurator configurator)
   {
      super(parent, style, configurator.setSelectionButtonToolTip(LocalizationHelper.getI18n(MultiEventSelector.class).tr("Select events")));
      setText(i18n.tr("<any>"));
   }

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractSelector#selectionButtonHandler()
    */
   @Override
   protected void selectionButtonHandler()
   {
      MultiEventSelectionDialog dlg = new MultiEventSelectionDialog(getShell(), eventCodes);
      if (dlg.open() == Window.OK)
      {
         EventTemplate[] events = dlg.getSelectedEvents();
         int[] prevCodes = eventCodes;

         if (events.length > 0)
         {
            eventCodes = new int[events.length];
            StringBuilder sb = new StringBuilder();
            for(int i = 0; i < events.length; i++)
            {
               eventCodes[i] = events[i].getCode();
               if (i > 0)
                  sb.append(", ");
               sb.append(events[i].getName());
            }

            if (events.length <= 3)
            {
               setText(sb.toString());
            }
            else
            {
               setText(String.format(i18n.tr("%d events selected"), events.length));
            }
            getTextControl().setToolTipText(sb.toString());
         }
         else
         {
            eventCodes = new int[0];
            setText(i18n.tr("<any>"));
            getTextControl().setToolTipText(null);
         }

         if (!java.util.Arrays.equals(prevCodes, eventCodes))
            fireModifyListeners();
      }
   }

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractSelector#clearButtonHandler()
    */
   @Override
   protected void clearButtonHandler()
   {
      if (eventCodes.length == 0)
         return;

      eventCodes = new int[0];
      setText(i18n.tr("<any>"));
      getTextControl().setToolTipText(null);
      fireModifyListeners();
   }

   /**
    * Get selected event codes.
    *
    * @return array of selected event codes (empty array if no filter)
    */
   public int[] getEventCodes()
   {
      return eventCodes;
   }

   /**
    * Set selected event codes.
    *
    * @param codes array of event codes to select
    */
   public void setEventCodes(int[] codes)
   {
      if (codes == null || codes.length == 0)
      {
         eventCodes = new int[0];
         setText(i18n.tr("Any"));
         getTextControl().setToolTipText(null);
         return;
      }

      eventCodes = codes.clone();
      StringBuilder sb = new StringBuilder();

      for(int i = 0; i < codes.length; i++)
      {
         EventTemplate evt = Registry.getSession().findEventTemplateByCode(codes[i]);

         if (i > 0)
            sb.append(", ");

         if (evt != null)
            sb.append(evt.getName());
         else
            sb.append("[").append(codes[i]).append("]");
      }

      if (codes.length <= 3)
      {
         setText(sb.toString());
      }
      else
      {
         setText(String.format(i18n.tr("%d events selected"), codes.length));
      }
      getTextControl().setToolTipText(sb.toString());
   }
}
