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
package org.netxms.reporting.nxcl;

import java.lang.reflect.Constructor;
import java.net.URL;
import java.util.Map;
import org.netxms.reporting.ReportClassLoader;
import org.netxms.reporting.Server;
import org.netxms.reporting.extensions.NXCLDataSource;
import org.netxms.reporting.services.ReportManager;
import org.netxms.reporting.tools.ThreadLocalReportInfo;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import net.sf.jasperreports.engine.JRDataSource;
import net.sf.jasperreports.engine.JRDataset;
import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRQueryChunk;
import net.sf.jasperreports.engine.JRValueParameter;
import net.sf.jasperreports.engine.JasperReportsContext;
import net.sf.jasperreports.engine.query.JRQueryExecuter;

/**
 * Custom query executor for reading data from NetXMS server via API
 */
public class NXCLQueryExecutor implements JRQueryExecuter
{
   private static final Logger log = LoggerFactory.getLogger(NXCLQueryExecutor.class);

   private JRDataset dataset;
   private Map<String, ? extends JRValueParameter> parametersMap;

   protected NXCLQueryExecutor(JasperReportsContext jasperReportsContext, JRDataset dataset,
         Map<String, ? extends JRValueParameter> parametersMap)
   {
      this.dataset = dataset;
      this.parametersMap = parametersMap;
   }

   /**
    * @see net.sf.jasperreports.engine.query.JRQueryExecuter#createDatasource()
    */
   @SuppressWarnings("unchecked")
   @Override
   public JRDataSource createDatasource() throws JRException
   {
      String reportLocation = ThreadLocalReportInfo.getReportLocation();

      try
      {
         URL[] urls = { new URL("file:" + reportLocation) };
         @SuppressWarnings("resource")
         ReportClassLoader classLoader = new ReportClassLoader(urls, getClass().getClassLoader());
         Class<NXCLDataSource> aClass = (Class<NXCLDataSource>)classLoader.loadClass("report.DataSource");
         Constructor<NXCLDataSource> constructor = aClass.getConstructor(JRDataset.class, Map.class);
         NXCLDataSource dataSource = constructor.newInstance(dataset, parametersMap);

         JRQueryChunk chunk = dataset.getQuery().getChunks()[0];
         dataSource.setQuery(chunk.getText().trim());

         Server server = ThreadLocalReportInfo.getServer();
         String authToken = getAuthToken();
         if (authToken != null)
            dataSource.connect(server.getConfigurationProperty("netxms.server.hostname", "localhost"), authToken);
         else
            dataSource.connect(server.getConfigurationProperty("netxms.server.hostname", "localhost"),
                  server.getConfigurationProperty("netxms.server.login", "admin"),
                  server.getConfigurationProperty("netxms.server.password", ""));

         return dataSource;
      }
      catch(Exception e)
      {
         log.error("Cannot load report data source", e);
      }
      return null;
   }

   /**
    * Get authentication token from report parameters.
    *
    * @return authentication token or null
    */
   private String getAuthToken()
   {
      JRValueParameter p = parametersMap.get(ReportManager.AUTH_TOKEN_KEY);
      if (p == null)
         return null;
      Object v = p.getValue();
      return (v instanceof String) ? (String)v : null;
   }

   /**
    * @see net.sf.jasperreports.engine.query.JRQueryExecuter#close()
    */
   @Override
   public void close()
   {
   }

   /**
    * @see net.sf.jasperreports.engine.query.JRQueryExecuter#cancelQuery()
    */
   @Override
   public boolean cancelQuery() throws JRException
   {
      log.warn("Query cancellation requested");
      return false;
   }
}
