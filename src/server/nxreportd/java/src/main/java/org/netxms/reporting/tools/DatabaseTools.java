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
package org.netxms.reporting.tools;

import java.sql.Connection;
import java.sql.Statement;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Database tools
 */
public class DatabaseTools
{
   private static final Logger logger = LoggerFactory.getLogger(DatabaseTools.class);

   /**
    * Drop temporary table
    *
    * @param dbConnection database connection
    * @param localParameters report local parameters
    * @param key key for parameter holding table name
    */
   public static void dropTemporaryTable(Connection dbConnection, Map<String, Object> localParameters, String key)
   {
      String tableName = (String)localParameters.get(key);
      if ((tableName == null) || tableName.isEmpty())
         return;

      logger.debug("Drop temporary table " + tableName);
      try
      {
         Statement stmt = dbConnection.createStatement();
         stmt.execute("DROP TABLE " + tableName);
         stmt.close();
      }
      catch(Exception e)
      {
         logger.error("Cannot drop temporary table " + tableName, e);
      }
   }

}
