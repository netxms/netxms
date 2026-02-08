/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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

import java.util.List;
import java.util.UUID;
import org.netxms.client.reporting.ReportResult;
import org.netxms.reporting.Server;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Housekeeper service
 */
public class Housekeeper
{
   private static final long HOUSEKEEPING_INTERVAL = 3600000L; // 1 hour

   private static final Logger logger = LoggerFactory.getLogger(Housekeeper.class);

   private Server server;
   private boolean running = false;
   private Thread workerThread = null;

   /**
    * Constructor
    * 
    * @param server server reference
    * @param reportManager report manager reference
    */
   public Housekeeper(Server server)
   {
      this.server = server;
   }

   /**
    * Start housekeeper thread
    */
   public void start()
   {
      running = true;
      workerThread = new Thread(() -> {
         logger.info("Housekeeper thread started");
         while(running)
         {
            try
            {
               performHousekeeping();
               Thread.sleep(HOUSEKEEPING_INTERVAL);
            }
            catch(InterruptedException e)
            {
               break;
            }
         }
         logger.info("Housekeeper thread stopped");
      });
      workerThread.start();
   }

   /**
    * Stop housekeeper thread
    */
   public void stop()
   {
      running = false;
      workerThread.interrupt();
      try
      {
         workerThread.join();
      }
      catch(InterruptedException e)
      {
         logger.error("Housekeeper stop interrupted", e);
      }
   }

   /**
    * Perform housekeeping tasks
    */
   private void performHousekeeping()
   {
      logger.info("Running housekeeping tasks");

      // Default to 10 years to avoid accidental data loss when property is not set or not yet received from server
      long retentionTime = server.getConfigurationPropertyAsInt("nxreportd.resultsRetenstionTime", 3650);
      logger.info("Report results retention time is set to " + retentionTime + " days");

      ReportManager reportManager = server.getReportManager();
      List<UUID> reports = reportManager.listReports();
      for(UUID reportId : reports)
      {
         List<ReportResult> results = reportManager.listResults(reportId, 0);
         for(ReportResult r : results)
         {
            if (r.getExecutionTime().getTime() < System.currentTimeMillis() - retentionTime * 86400000L)
            {
               logger.info("Deleting job result " + r.getJobId() + " of report " + reportId);
               reportManager.deleteResult(reportId, r.getJobId());
            }
         }
      }
   }
}
