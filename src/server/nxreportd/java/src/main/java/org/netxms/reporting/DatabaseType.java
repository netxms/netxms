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
package org.netxms.reporting;

/**
 * Database type
 */
public enum DatabaseType
{
   INFORMIX("com.informix.jdbc.IfxDriver"),
   MARIADB("com.mariadb.jdbc.Driver"),
   MSSQL("com.microsoft.sqlserver.jdbc.SQLServerDriver"),
   MYSQL("com.mysql.jdbc.Driver"),
   ORACLE("oracle.jdbc.driver.OracleDriver"),
   POSTGRESQL("org.postgresql.Driver");

   private final String driver;

   DatabaseType(String driver)
   {
      this.driver = driver;
   }

   public String getDriver()
   {
      return driver;
   }

   public static DatabaseType lookup(String key)
   {
      if (key == null)
         return null;

      String lowerKey = key.toLowerCase();
      if (lowerKey.equals("postgresql") || lowerKey.contains("pgsql"))
      {
         return POSTGRESQL;
      }
      else if (lowerKey.contains("oracle"))
      {
         return ORACLE;
      }
      else if (lowerKey.contains("mssql"))
      {
         return MSSQL;
      }
      else if (lowerKey.contains("mariadb"))
      {
         return MYSQL;
      }
      else if (lowerKey.contains("mysql"))
      {
         return MYSQL;
      }
      else if (lowerKey.contains("informix"))
      {
         return INFORMIX;
      }
      else if (lowerKey.contains("sqlite"))
      {
         throw new RuntimeException("SQLite not supported");
      }
      return null;
   }
}
