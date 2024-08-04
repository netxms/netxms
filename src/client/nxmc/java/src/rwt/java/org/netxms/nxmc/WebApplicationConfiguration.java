/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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

import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;
import org.eclipse.core.internal.runtime.RuntimeLog;
import org.eclipse.core.runtime.ILogListener;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.rap.rwt.application.Application;
import org.eclipse.rap.rwt.application.Application.OperationMode;
import org.eclipse.rap.rwt.application.ApplicationConfiguration;
import org.eclipse.rap.rwt.application.ExceptionHandler;
import org.eclipse.rap.rwt.client.WebClient;
import org.eclipse.rap.rwt.internal.application.ApplicationImpl;
import org.eclipse.rap.rwt.internal.resources.ContentBuffer;
import org.eclipse.rap.rwt.service.ResourceLoader;
import org.eclipse.swt.SWT;
import org.netxms.client.services.ServiceManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import io.github.secretx33.resourceresolver.PathMatchingResourcePatternResolver;
import io.github.secretx33.resourceresolver.Resource;
import io.github.secretx33.resourceresolver.ResourcePatternResolver;

/**
 * Web application configuration
 */
public class WebApplicationConfiguration implements ApplicationConfiguration
{
   private static Logger logger = LoggerFactory.getLogger(WebApplicationConfiguration.class);

   private final ContentBuffer concatenatedScript = new ContentBuffer();
   private final ResourceLoader genericResourceLoader = new ResourceLoader() {
      @Override
      public InputStream getResourceAsStream(String resourceName) throws IOException
      {
         return getClass().getResourceAsStream("/" + resourceName);
      }
   };

   /**
    * @see org.eclipse.rap.rwt.application.ApplicationConfiguration#configure(org.eclipse.rap.rwt.application.Application)
    */
   @Override
   public void configure(Application app)
   {
      addJsLibrary("/js/dom-to-image-more.js");
      addJsLibrary("/js/download.js");
      addJsLibrary("/js/longpress.js");
      addJsLibrary("/js/msgproxy.js");
      addJsLibrary("/js/rwt-util.js");
      app.addResource(SWT.getVersion() + "/nxmc-library.js", new ResourceLoader() {
         @Override
         public InputStream getResourceAsStream(String resourceName)
         {
            return concatenatedScript.getContentAsStream();
         }
      });
      ((ApplicationImpl)app).getApplicationContext().getStartupPage().addJsLibrary("rwt-resources/" + SWT.getVersion() + "/nxmc-library.js");

      registerAllResources(app, "vncviewer");

      app.addServiceHandler(DownloadServiceHandler.ID, new DownloadServiceHandler());
      app.addServiceHandler(VideoServiceHandler.ID, new VideoServiceHandler());

      app.addStyleSheet("org.netxms.themes.light", "/themes/light.css");

      Map<String, String> properties = new HashMap<>();
      properties.put(WebClient.THEME_ID, "org.netxms.themes.light");
      app.addEntryPoint("/nxmc-light.app", Startup.class, properties);

      app.setOperationMode(OperationMode.SWT_COMPATIBILITY);
      app.setExceptionHandler(new ExceptionHandler() {
         @Override
         public void handleException(Throwable t)
         {
            logger.error("Unhandled event loop exception", t);
         }
      });
      RuntimeLog.addLogListener(new ILogListener() {
         @Override
         public void logging(IStatus status, String plugin)
         {
            switch(status.getSeverity())
            {
               case IStatus.ERROR:
                  logger.error(status.getMessage(), status.getException());
                  break;
               case IStatus.WARNING:
                  logger.warn(status.getMessage(), status.getException());
                  break;
               case IStatus.INFO:
                  logger.info(status.getMessage(), status.getException());
                  break;
            }
         }
      });

      ServiceManager.registerClassLoader(getClass().getClassLoader());
   }

   /**
    * Register all resources starting at given path.
    *
    * @param app application
    * @param basePath base resource path (without leading /)
    */
   private void registerAllResources(Application app, String basePath)
   {
      logger.debug("Scanning resource path \"{}\"", basePath);
      ResourcePatternResolver resolver = new PathMatchingResourcePatternResolver();
      try
      {
         for(Resource r : resolver.getResources("classpath:" + basePath + "/**/*"))
         {
            String uri = r.getURI().toString();
            if (!uri.endsWith("/"))
            {
               int i = uri.lastIndexOf(basePath + "/");
               if (i > 0)
               {
                  String resourcePath = uri.substring(i);
                  app.addResource(resourcePath, genericResourceLoader);
                  logger.debug("Registered resource \"{}\"", resourcePath);
               }
            }
         }
      }
      catch(IOException e)
      {
         logger.error("Error registering resources", e);
      }
   }

   /**
    * Add JS library script
    *
    * @param path resource path
    */
   private void addJsLibrary(String path)
   {
      try
      {
         concatenatedScript.append(getClass().getClassLoader().getResourceAsStream(path));
      }
      catch(IOException e)
      {
         logger.error("Cannot add JS library", e);
      }
   }
}
