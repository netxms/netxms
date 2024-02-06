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
package org.netxms.reporting.extensions;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.util.HashMap;
import java.util.Map;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.users.ResponsibleUser;

/**
 * Prepare temporary table with snapshot of all reponsible users for each node
 */
public class PrepareResponsibleUsers extends ExecutionHook
{
   /**
    * @see org.netxms.reporting.extensions.ExecutionHook#isServerAccessRequired()
    */
   @Override
   public boolean isServerAccessRequired()
   {
      return true;
   }

   /**
    * @see org.netxms.reporting.extensions.ExecutionHook#run(java.util.Map, java.sql.Connection)
    */
   @Override
   public void run(Map<String, Object> parameters, Connection dbConnection) throws Exception
   {
      String tag = (String)parameters.get("responsible_users_tag");
      String tableName = "ru_" + System.currentTimeMillis();
      dbConnection.createStatement().executeUpdate("CREATE TABLE " + tableName + " (node_id integer not null,user_id integer not null,tag varchar(31) null,PRIMARY KEY(node_id,user_id))");
      dbConnection.setAutoCommit(false);
      PreparedStatement stmt = dbConnection.prepareStatement("INSERT INTO " + tableName + " (node_id,user_id,tag) VALUES (?,?,?)");
      for(AbstractObject o : session.getAllObjects())
      {
         if (o instanceof Node)
         {
            Map<Integer, ResponsibleUser> users = new HashMap<>();
            collectResponsibleUsers(o, users, tag);
            if (!users.isEmpty())
            {
               stmt.setInt(1, (int)o.getObjectId());
               for(ResponsibleUser u : users.values())
               {
                  stmt.setInt(2, u.userId);
                  stmt.setString(3, u.tag);
                  stmt.executeUpdate();
               }
            }
         }
      }
      dbConnection.commit();
      stmt.close();
      dbConnection.setAutoCommit(true);
      parameters.put("responsible_users_table", tableName);
   }

   /**
    * Collect all responsible users for given object
    *
    * @param object object to process
    * @param users set of user IDs
    * @param tag tag to match or null to match any tag
    */
   private void collectResponsibleUsers(AbstractObject object, Map<Integer, ResponsibleUser> users, String tag)
   {
      for(ResponsibleUser u : object.getResponsibleUsers())
      {
         if ((tag == null) || tag.equalsIgnoreCase(u.tag))
            users.put(u.userId, u);
      }
      for(AbstractObject p : object.getParentsAsArray())
         collectResponsibleUsers(p, users, tag);
   }
}
