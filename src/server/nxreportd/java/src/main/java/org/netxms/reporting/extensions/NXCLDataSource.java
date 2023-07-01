/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import net.sf.jasperreports.engine.JRDataSource;
import net.sf.jasperreports.engine.JRDataset;
import net.sf.jasperreports.engine.JRValueParameter;

/**
 * Custom data source for reading data from NetXMS server via API
 */
public abstract class NXCLDataSource extends GenericExtension implements JRDataSource
{
   protected JRDataset dataset;
   protected Map<String, ? extends JRValueParameter> parameters;

   protected NXCLDataSource(JRDataset dataset, Map<String, ? extends JRValueParameter> parameters)
   {
      super();
      this.dataset = dataset;
      this.parameters = parameters;
   }

   protected Object getParameterValue(String name)
   {
      return parameters == null ? null : parameters.get(name).getValue();
   }

   /**
    * @see org.netxms.reporting.extensions.GenericExtension#onConnect(org.netxms.client.NXCSession)
    */
   @Override
   protected void onConnect(NXCSession session) throws IOException, NXCException
   {
      loadData(session);
   }

   /**
    * Load necessary data from server.
    *
    * @param session client session
    * @throws IOException
    * @throws NXCException
    */
   public abstract void loadData(NXCSession session) throws IOException, NXCException;

   public abstract void setQuery(String query);
}
