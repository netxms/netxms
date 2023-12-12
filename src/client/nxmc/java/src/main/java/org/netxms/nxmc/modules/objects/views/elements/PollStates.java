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
package org.netxms.nxmc.modules.objects.views.elements;

import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.PollState;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.interfaces.PollingTarget;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.xnap.commons.i18n.I18n;

/**
 * Poll states
 */
public class PollStates extends TableElement
{
   private final I18n i18n = LocalizationHelper.getI18n(PollStates.class);

   private Map<String, String> typeNames = new HashMap<>();

   /**
    * Create element.
    *
    * @param parent parent composite
    * @param anchor anchor element
    * @param objectView owning view
    */
   public PollStates(Composite parent, OverviewPageElement anchor, ObjectView objectView)
   {
      super(parent, anchor, objectView);
      typeNames.put("autobind", i18n.tr("Autobind"));
      typeNames.put("configuration", i18n.tr("Configuration"));
      typeNames.put("discovery", i18n.tr("Network discovery"));
      typeNames.put("icmp", i18n.tr("ICMP"));
      typeNames.put("instance", i18n.tr("Instance discovery"));
      typeNames.put("map", i18n.tr("Map update"));
      typeNames.put("routing", i18n.tr("Routing table"));
      typeNames.put("status", i18n.tr("Status"));
      typeNames.put("topology", i18n.tr("Topology"));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      return (object instanceof PollingTarget) && (((PollingTarget)object).getPollStates() != null);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.TableElement#fillTable()
    */
   @Override
   protected void fillTable()
   {
      for(PollState s : ((PollingTarget)getObject()).getPollStates())
      {
         String type = typeNames.get(s.getPollType());
         if (type == null)
            type = s.getPollType();

         if (s.isPending())
         {
            addPair(type, i18n.tr("pending"));
         }
         else
         {
            Date lastCompleted = s.getLastCompleted();
            addPair(type, (lastCompleted.getTime() > 0) ? DateFormatFactory.getTimeFormat().format(lastCompleted) : i18n.tr("never"));
         }
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
   @Override
   protected String getTitle()
   {
      return "Polls";
   }
}
