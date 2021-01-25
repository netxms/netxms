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
package org.netxms.reporting.extensions;

import java.io.IOException;
import java.util.Map;
import net.sf.jasperreports.engine.JRDataSource;
import net.sf.jasperreports.engine.JRDataset;
import net.sf.jasperreports.engine.JRValueParameter;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Custom data source for reading data from NetXMS server via API
 */
public abstract class NXCLDataSource implements JRDataSource
{
   private static final Logger log = LoggerFactory.getLogger(NXCLDataSource.class);

   private static final int[] PROTOCOL_COMPONENTS = { ProtocolVersion.INDEX_FULL };

   protected JRDataset dataset;
   protected Map<String, ? extends JRValueParameter> parameters;

   protected NXCLDataSource(JRDataset dataset, Map<String, ? extends JRValueParameter> parameters)
   {
      this.dataset = dataset;
      this.parameters = parameters;
   }

   /**
    * Connect to NetXMS server.
    *
    * @param server server host name
    * @param login login name
    * @param password password
    */
   public void connect(String server, String login, String password)
   {
      log.debug("Connecting to NetXMS server at " + server + " as " + login);
      NXCSession session = new NXCSession(server);
      try
      {
         session.connect(PROTOCOL_COMPONENTS);
         session.login(login, password);
         session.syncEventTemplates();
         session.syncObjects();
         loadData(session);
      }
      catch(Exception e)
      {
         log.error("Cannot connect to NetXMS", e);
      }
   }

   protected Object getParameterValue(String name)
   {
      return parameters == null ? null : parameters.get(name).getValue();
   }

   public abstract void loadData(NXCSession session) throws IOException, NXCException;

   public abstract void setQuery(String query);
}
