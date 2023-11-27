/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.logviewer.views;

import java.util.ArrayList;
import java.util.List;
import java.util.ServiceLoader;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PerspectiveConfiguration;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.services.ServerLogDescriptor;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Log viewer perspective
 */
public class LogViewerPerspective extends Perspective
{
   private static final Logger logger = LoggerFactory.getLogger(LogViewerPerspective.class);
   private final I18n i18n = LocalizationHelper.getI18n(LogViewerPerspective.class);

   private List<ServerLogDescriptor> logs = new ArrayList<ServerLogDescriptor>();

   /**
    * The constructor.
    */
   public LogViewerPerspective()
   {
      super("Logs", LocalizationHelper.getI18n(LogViewerPerspective.class).tr("Logs"), ResourceManager.getImage("icons/perspective-logs.png"));

      // Register default logs
      registerStandardLogViewer(i18n.tr("Alarms"), "AlarmLog");
      registerStandardLogViewer(i18n.tr("Asset Changes"), "AssetChaneLog");
      registerStandardLogViewer(i18n.tr("Audit"), "AuditLog");
      registerStandardLogViewer(i18n.tr("Events"), "EventLog");
      registerStandardLogViewer(i18n.tr("Maintenance Journal"), "MaintenanceJournal");
      registerStandardLogViewer(i18n.tr("Notifications"), "NotificationLog");
      registerStandardLogViewer(i18n.tr("Server Action Executions"), "ServerActionExecutionLog");
      registerStandardLogViewer(i18n.tr("SNMP Traps"), "SnmpTrapLog");
      registerStandardLogViewer(i18n.tr("Syslog"), "syslog");
      registerStandardLogViewer(i18n.tr("Windows Events"), "WindowsEventLog");

      // Register additional logs
      ServiceLoader<ServerLogDescriptor> loader = ServiceLoader.load(ServerLogDescriptor.class, getClass().getClassLoader());
      for(ServerLogDescriptor e : loader)
         logs.add(e);

      logs.sort((ServerLogDescriptor l1, ServerLogDescriptor l2) -> l1.getDisplayName().compareToIgnoreCase(l2.getDisplayName()));
   }

   /**
    * Register standard log viewer
    *
    * @param displayName display name for viewer
    * @param logName log name
    */
   private void registerStandardLogViewer(final String displayName, final String logName)
   {
      logs.add(new ServerLogDescriptor() {
         @Override
         public LogViewer createView()
         {
            return new LogViewer(displayName, logName);
         }

         @Override
         public String getDisplayName()
         {
            return displayName;
         }

         @Override
         public String getRequiredComponentId()
         {
            return null;
         }
      });
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configurePerspective(org.netxms.nxmc.base.views.PerspectiveConfiguration)
    */
   @Override
   protected void configurePerspective(PerspectiveConfiguration configuration)
   {
      super.configurePerspective(configuration);
      configuration.hasNavigationArea = false;
      configuration.multiViewMainArea = true;
      configuration.hasSupplementalArea = false;
      configuration.priority = 100;
   }

   /**
    * @see org.netxms.nxmc.base.views.Perspective#configureViews()
    */
   @Override
   protected void configureViews()
   {
      NXCSession session = Registry.getSession();
      for(ServerLogDescriptor log : logs)
      {
         String componentId = log.getRequiredComponentId();
         if ((componentId == null) || session.isServerComponentRegistered(componentId))
         {
            LogViewer view = log.createView();
            addMainView(view);
            logger.debug("Added logs perspective view \"" + view.getName() + "\"");
         }
         else
         {
            logger.debug("Logs perspective view \"" + log.getDisplayName() + "\" blocked by component filter");
         }
      }
   }
}
