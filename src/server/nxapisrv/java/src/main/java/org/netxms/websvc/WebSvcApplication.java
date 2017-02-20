/**
 * Web services API for NetXMS
 * Copyright (c) 2017 Raden Solutions
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
