/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.websvc;

import java.util.Arrays;
import java.util.HashSet;
import java.util.ServiceLoader;
import org.netxms.client.services.ServiceManager;
import org.netxms.websvc.handlers.AccessIntegrationTools;
import org.netxms.websvc.handlers.AlarmComments;
import org.netxms.websvc.handlers.Alarms;
import org.netxms.websvc.handlers.BindHandler;
import org.netxms.websvc.handlers.BindToHandler;
import org.netxms.websvc.handlers.BusinessServiceChecks;
import org.netxms.websvc.handlers.BusinessServiceTickets;
import org.netxms.websvc.handlers.BusinessServiceUptime;
import org.netxms.websvc.handlers.ChangeMgmtStateHandler;
import org.netxms.websvc.handlers.ChangeZoneHandler;
import org.netxms.websvc.handlers.DCObjectLastValue;
import org.netxms.websvc.handlers.DCObjectPollHandler;
import org.netxms.websvc.handlers.DCObjectStatusChangeHandler;
import org.netxms.websvc.handlers.DataCollectionConfigurationHandler;
import org.netxms.websvc.handlers.DataCollectionObjectHandler;
import org.netxms.websvc.handlers.DataCollectionQueryHandler;
import org.netxms.websvc.handlers.Events;
import org.netxms.websvc.handlers.FindMacAddress;
import org.netxms.websvc.handlers.GrafanaAlarms;
import org.netxms.websvc.handlers.GrafanaDataCollection;
import org.netxms.websvc.handlers.HistoricalData;
import org.netxms.websvc.handlers.InfoHandler;
import org.netxms.websvc.handlers.LastValues;
import org.netxms.websvc.handlers.MapAccess;
import org.netxms.websvc.handlers.NotificationHandler;
import org.netxms.websvc.handlers.ChangeMaintenanceStateHandler;
import org.netxms.websvc.handlers.ObjectToolOutputHandler;
import org.netxms.websvc.handlers.ObjectTools;
import org.netxms.websvc.handlers.Objects;
import org.netxms.websvc.handlers.PersistentStorage;
import org.netxms.websvc.handlers.Polls;
import org.netxms.websvc.handlers.PollsOutputHandler;
import org.netxms.websvc.handlers.PredefinedGraphs;
import org.netxms.websvc.handlers.PushDataHandler;
import org.netxms.websvc.handlers.RootHandler;
import org.netxms.websvc.handlers.Sessions;
import org.netxms.websvc.handlers.SummaryTableAdHoc;
import org.netxms.websvc.handlers.UnbindFromHandler;
import org.netxms.websvc.handlers.UnbindHandler;
import org.netxms.websvc.handlers.UserAgentNotifications;
import org.netxms.websvc.handlers.UserPassword;
import org.netxms.websvc.handlers.Users;
import org.restlet.Application;
import org.restlet.Restlet;
import org.restlet.data.Method;
import org.restlet.routing.Router;
import org.restlet.service.CorsService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * NetXMS web service API application
 */
public class WebSvcApplication extends Application
{
   private Logger log = LoggerFactory.getLogger(WebSvcApplication.class);

   /**
    * Default constructor
    */
   public WebSvcApplication()
   {
      ServiceManager.registerClassLoader(getClass().getClassLoader());
      CorsService corsService = new CorsService();
      corsService.setAllowedOrigins(new HashSet<String>(Arrays.asList("*")));
      corsService.setAllowedCredentials(true);
      corsService.setAllowedHeaders(new HashSet<String>(Arrays.asList("Content-Type", "Authorization")));
      corsService.setDefaultAllowedMethods(new HashSet<Method>(Arrays.asList(Method.DELETE, Method.GET, Method.OPTIONS, Method.POST, Method.PUT)));
      getServices().add(corsService);
      WebSvcStatusService statusService = new WebSvcStatusService();
      statusService.setOverwriting(true);
      setStatusService(statusService);
   }

   /**
    * @see org.restlet.Application#createInboundRoot()
    */
   @Override
   public synchronized Restlet createInboundRoot()
   {
      log.debug("createInboundRoot() called");
      Router router = new Router(getContext());
      router.attach("/", RootHandler.class);
      router.attach("/alarms", Alarms.class);
      router.attach("/alarms/{id}", Alarms.class);
      router.attach("/alarms/{alarm-id}/comments", AlarmComments.class);
      router.attach("/alarms/{alarm-id}/comments/{id}", AlarmComments.class);
      router.attach("/authenticate", AccessIntegrationTools.class);
      router.attach("/events", Events.class);
      router.attach("/find/mac-address", FindMacAddress.class);
      router.attach("/grafana/alarms", GrafanaAlarms.class);
      router.attach("/grafana/datacollection", GrafanaDataCollection.class);
      router.attach("/info", InfoHandler.class);
      router.attach("/notifications", NotificationHandler.class);
      router.attach("/objects", Objects.class);
      router.attach("/objects/{object-id}", Objects.class);
      router.attach("/objects/{object-id}/anonymous-access", MapAccess.class);
      router.attach("/objects/{object-id}/bind", BindHandler.class);
      router.attach("/objects/{object-id}/bind-to", BindToHandler.class);
      router.attach("/objects/{object-id}/change-zone", ChangeZoneHandler.class);
      router.attach("/objects/{object-id}/checks", BusinessServiceChecks.class);
      router.attach("/objects/{object-id}/checks/{id}", BusinessServiceChecks.class);
      router.attach("/objects/{object-id}/data-collection", DataCollectionConfigurationHandler.class);
      router.attach("/objects/{object-id}/data-collection/set-status", DCObjectStatusChangeHandler.class);
      router.attach("/objects/{object-id}/data-collection/query", DataCollectionQueryHandler.class);
      router.attach("/objects/{object-id}/data-collection/{id}", DataCollectionObjectHandler.class);
      router.attach("/objects/{object-id}/data-collection/{id}/force-poll", DCObjectPollHandler.class);
      router.attach("/objects/{object-id}/data-collection/{id}/last-value", DCObjectLastValue.class);
      router.attach("/objects/{object-id}/data-collection/{id}/values", HistoricalData.class);
      router.attach("/objects/{object-id}/last-values", LastValues.class);
      router.attach("/objects/{object-id}/object-tools", ObjectTools.class);
      router.attach("/objects/{object-id}/object-tools/output/{id}", ObjectToolOutputHandler.class);
      router.attach("/objects/{object-id}/polls", Polls.class);
      router.attach("/objects/{object-id}/polls/output/{id}", PollsOutputHandler.class);
      router.attach("/objects/{object-id}/set-maintenance", ChangeMaintenanceStateHandler.class);
      router.attach("/objects/{object-id}/set-managed", ChangeMgmtStateHandler.class);
      router.attach("/objects/{object-id}/tickets", BusinessServiceTickets.class);
      router.attach("/objects/{object-id}/unbind", UnbindHandler.class);
      router.attach("/objects/{object-id}/unbind-from", UnbindFromHandler.class);
      router.attach("/objects/{object-id}/uptime", BusinessServiceUptime.class);
      router.attach("/persistent-storage", PersistentStorage.class);
      router.attach("/persistent-storage/{id}", PersistentStorage.class);
      router.attach("/predefined-graphs", PredefinedGraphs.class);
      router.attach("/push-data", PushDataHandler.class);
      router.attach("/sessions", Sessions.class);
      router.attach("/sessions/{id}", Sessions.class);
      router.attach("/summary-table/ad-hoc", SummaryTableAdHoc.class);
      router.attach("/user-agent-notifications", UserAgentNotifications.class);
      router.attach("/users", Users.class);
      router.attach("/users/{id}", Users.class);
      router.attach("/users/{id}/password", UserPassword.class);

      ServiceLoader<WebSvcExtension> loader = ServiceLoader.load(WebSvcExtension.class);
      for(WebSvcExtension l : loader)
      {
         log.debug("Calling web service extension " + l.toString());
         try
         {
            l.createInboundRoot(router);
         }
         catch(Exception e)
         {
            log.error("Exception in web service extension", e);
         }
      }
      
      return router;
   }
}
