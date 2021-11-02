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
package org.netxms.nxmc;

import org.eclipse.rap.rwt.application.Application;
import org.eclipse.rap.rwt.application.Application.OperationMode;
import org.eclipse.rap.rwt.application.ApplicationConfiguration;
import org.eclipse.rap.rwt.application.ExceptionHandler;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Web application configuration
 */
public class WebApplicationConfiguration implements ApplicationConfiguration
{
   private static Logger logger = LoggerFactory.getLogger(WebApplicationConfiguration.class);

   /**
    * @see org.eclipse.rap.rwt.application.ApplicationConfiguration#configure(org.eclipse.rap.rwt.application.Application)
    */
   @Override
   public void configure(Application app)
   {
      app.addEntryPoint("/nxmc", Startup.class, null);
      app.setOperationMode(OperationMode.SWT_COMPATIBILITY);
      app.setExceptionHandler(new ExceptionHandler() {
         @Override
         public void handleException(Throwable t)
         {
            logger.error("Unhandled event loop exception", t);
         }
      });
   }
}
