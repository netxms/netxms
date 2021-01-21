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
package org.netxms.reporting.services;

import java.io.File;
import java.io.IOException;
import java.util.List;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import name.pachler.nio.file.ClosedWatchServiceException;
import name.pachler.nio.file.FileSystems;
import name.pachler.nio.file.Path;
import name.pachler.nio.file.Paths;
import name.pachler.nio.file.StandardWatchEventKind;
import name.pachler.nio.file.WatchEvent;
import name.pachler.nio.file.WatchKey;
import name.pachler.nio.file.WatchService;

/**
 * File monitor
 */
public class FileMonitor
{
   private static final Logger logger = LoggerFactory.getLogger(FileMonitor.class);

   private WatchService watchService;
   private Path watchedPath;
   private Callback callback;

   /**
    * Create new file monitor.
    *
    * @param directory directory to monitor
    * @param callback callback to be called when change is detected
    */
   public FileMonitor(File directory, Callback callback)
   {
      this.callback = callback;
      watchService = FileSystems.getDefault().newWatchService();
      watchedPath = Paths.get(directory.getAbsolutePath());
   }

   /**
    * Start file monitor.
    */
   public void start()
   {
      try
      {
         watchedPath.register(watchService, StandardWatchEventKind.ENTRY_CREATE, StandardWatchEventKind.ENTRY_DELETE);
         new Thread(new Runnable() {
            @Override
            public void run()
            {
               monitor();
            }
         }, "FileMonior").start();
      }
      catch(Exception e)
      {
         logger.error("Cannot start file monitor", e);
      }
   }

   /**
    * Stop file monitor.
    */
   public void stop()
   {
      try
      {
         watchService.close();
      }
      catch(IOException e)
      {
         logger.error("Unexpected error while stopping file monitor", e);
      }
   }

   /**
    * Monitoring loop
    */
   private void monitor()
   {
      logger.info("File monitor started");
      while(true)
      {
         WatchKey signalledKey;
         try
         {
            signalledKey = watchService.take();
         }
         catch(InterruptedException e)
         {
            continue; // ignore
         }
         catch(ClosedWatchServiceException e)
         {
            logger.info("Watch service closed");
            break;
         }

         List<WatchEvent<?>> list = signalledKey.pollEvents();
         signalledKey.reset();

         for(WatchEvent<?> e : list)
         {
            if (e.kind() == StandardWatchEventKind.ENTRY_CREATE)
            {
               String name = e.context().toString();
               logger.debug("Entry " + name + " created in " + watchedPath.toString());
               callback.onCreate(name);
            }
            else if (e.kind() == StandardWatchEventKind.ENTRY_DELETE)
            {
               String name = e.context().toString();
               logger.debug("Entry " + name + " deleted from " + watchedPath.toString());
               callback.onDelete(name);
            }
            else if (e.kind() == StandardWatchEventKind.OVERFLOW)
            {
               logger.warn("Watch service overflow");
            }
         }
      }
      logger.info("File monitor stopped");
   }

   /**
    * Callback interface
    */
   public interface Callback
   {
      /**
       * Called when new entry is created.
       *
       * @param name entry name
       */
      public void onCreate(String name);

      /**
       * Called when entry is deleted
       *
       * @param name entry name
       */
      public void onDelete(String name);
   }
}
