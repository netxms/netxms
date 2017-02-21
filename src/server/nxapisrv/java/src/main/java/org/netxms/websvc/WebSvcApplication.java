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

import org.netxms.websvc.handlers.Alarms;
import org.netxms.websvc.handlers.Objects;
import org.netxms.websvc.handlers.Sessions;
import org.restlet.Application;
import org.restlet.Restlet;
import org.restlet.routing.Router;
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
      router.attach("/objects", Objects.class);
      router.attach("/objects/{id}", Objects.class);
      router.attach("/sessions", Sessions.class);
      router.attach("/sessions/{id}", Sessions.class);
      return router;
   }
}
