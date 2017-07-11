/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
import org.netxms.websvc.handlers.AccessIntegrationTools;
import org.netxms.websvc.handlers.Alarms;
import org.netxms.websvc.handlers.GrafanaAlarms;
import org.netxms.websvc.handlers.GrafanaDataCollection;
import org.netxms.websvc.handlers.HistoricalData;
import org.netxms.websvc.handlers.Objects;
import org.netxms.websvc.handlers.Sessions;
import org.netxms.websvc.handlers.SummaryTableAdHoc;
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
      CorsService corsService = new CorsService();
      corsService.setAllowedOrigins(new HashSet<String>(Arrays.asList("*")));
      corsService.setAllowedCredentials(true);
      corsService.setAllowedHeaders(new HashSet<String>(Arrays.asList("Content-Type", "Authorization")));
      corsService.setDefaultAllowedMethods(new HashSet<Method>(Arrays.asList(Method.DELETE, Method.GET, Method.OPTIONS, Method.POST, Method.PUT)));
      getServices().add(corsService);
      setStatusService(new WebSvcStatusService());
   }
   
   /* (non-Javadoc)
    * @see org.restlet.Application#createInboundRoot()
    */
   @Override
   public synchronized Restlet createInboundRoot()
   {
      log.debug("createInboundRoot() called");
      Router router = new Router(getContext());
      router.attach("/alarms", Alarms.class);
      router.attach("/alarms/{id}", Alarms.class);
      router.attach("/authenticate", AccessIntegrationTools.class);
      router.attach("/grafana/alarms", GrafanaAlarms.class);
      router.attach("/grafana/datacollection", GrafanaDataCollection.class);
      router.attach("/objects", Objects.class);
      router.attach("/objects/{object-id}", Objects.class);
      router.attach("/objects/{object-id}/datacollection/{id}/values", HistoricalData.class);
      router.attach("/sessions", Sessions.class);
      router.attach("/sessions/{id}", Sessions.class);
      router.attach("/summaryTable/adHoc", SummaryTableAdHoc.class);
      return router;
   }
}
