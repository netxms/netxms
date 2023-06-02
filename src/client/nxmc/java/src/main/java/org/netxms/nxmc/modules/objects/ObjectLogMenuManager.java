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
package org.netxms.nxmc.modules.objects;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logviewer.LogDeclaration;
import org.netxms.nxmc.modules.logviewer.WindowsLogDeclaration;
import org.netxms.nxmc.modules.logviewer.views.ObjectLogViewer;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Helper class for building object context menu
 */
public class ObjectLogMenuManager extends MenuManager
{
   private static final I18n i18n = LocalizationHelper.getI18n(ObjectLogMenuManager.class);

   private AbstractObject object;
   private long contextId;
   private ViewPlacement viewPlacement;

   private LogDeclaration alarmLog;
   private LogDeclaration eventLog;
   private LogDeclaration snmpTrapLog;
   private LogDeclaration syslogLog;
   private LogDeclaration windowsEventLog;

   /**
    * Create new menu manager for object's "Logs" menu
    *
    * @param view parent view
    * @param viewPlacement view placement
    * @param parent object to create menu for
    */
   public ObjectLogMenuManager(AbstractObject object, long contextId, ViewPlacement viewPlacement)
   {
      this.contextId = contextId;
      this.viewPlacement = viewPlacement;
      this.object = object;
      setMenuText(i18n.tr("&Logs"));     

      createLogDeclarations();

      addAction(this, alarmLog);
      addAction(this, eventLog);
      addAction(this, snmpTrapLog);
      addAction(this, syslogLog);
      addAction(this, windowsEventLog);
   }

   /**
    * Create declaration objects
    */
   protected void createLogDeclarations()
   {
      alarmLog = new LogDeclaration(i18n.tr("Alarms"), "AlarmLog", "source_object_id");
      eventLog = new LogDeclaration(i18n.tr("Events"), "EventLog", "event_source");
      snmpTrapLog = new LogDeclaration(i18n.tr("SNMP Traps"), "SnmpTrapLog", "object_id");
      syslogLog = new LogDeclaration(i18n.tr("Syslog"), "syslog", "source_object_id");
      windowsEventLog = new WindowsLogDeclaration();
   }

   /**
    * Add action to given menu manager
    *
    * @param manager menu manager
    * @param action action to add
    * @param filter filter for current selection
    */
   protected void addAction(IMenuManager manager, LogDeclaration logDeclaration)
   {
      if (logDeclaration.isApplicableForObject(object))
      {
         Action action = new Action(i18n.tr("Open {0} log", logDeclaration.getViewName()), ResourceManager.getImageDescriptor("icons/log-viewer/" + logDeclaration.getLogName() + ".png")) {
            @Override
            public void run()
            {
               openLong(logDeclaration);
            }
         };
         manager.add(action);
      }
   }
   
   /**
    * Open log view
    */
   private void openLong(LogDeclaration logDeclaration)
   {      
      Perspective p = viewPlacement.getPerspective();   
      ObjectLogViewer view = new ObjectLogViewer(logDeclaration, object, contextId);
      if (p != null)
      {
         p.addMainView(view, true, false);
      }
      else
      {
         PopOutViewWindow.open(view);
      }
   }
}
