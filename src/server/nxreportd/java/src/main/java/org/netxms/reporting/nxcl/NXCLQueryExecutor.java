package org.netxms.reporting.nxcl;

import net.sf.jasperreports.engine.JRDataSource;
import net.sf.jasperreports.engine.JRDataset;
import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRQueryChunk;
import net.sf.jasperreports.engine.JRValueParameter;
import net.sf.jasperreports.engine.JasperReportsContext;
import net.sf.jasperreports.engine.query.JRQueryExecuter;
import org.netxms.reporting.ReportClassLoader;
import org.netxms.reporting.Server;
import org.netxms.reporting.tools.ThreadLocalReportInfo;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Map;

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
         dataSource.connect(server.getConfigurationProperty("netxms.server.hostname", "localhost"),
               server.getConfigurationProperty("netxms.server.login", "admin"),
               server.getConfigurationProperty("netxms.server.password", ""));
         return dataSource;
      }
      catch(ClassNotFoundException e)
      {
         // ignore
      }
      catch(MalformedURLException e)
      {
         log.error("Can't load report data source", e);
      }
      catch(InstantiationException e)
      {
         log.error("Can't load report data source", e);
      }
      catch(IllegalAccessException e)
      {
         log.error("Can't load report data source", e);
      }
      catch(NoSuchMethodException e)
      {
         log.error("Can't load report data source", e);
      }
      catch(InvocationTargetException e)
      {
         log.error("Can't load report data source", e);
      }
      return null;
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
      System.out.println("cancelQuery");
      return false;
   }
}
