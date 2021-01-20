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

import java.io.IOException;
import java.io.InputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Properties;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import org.apache.commons.daemon.Daemon;
import org.apache.commons.daemon.DaemonContext;
import org.apache.commons.daemon.DaemonInitException;
import org.hibernate.SessionFactory;
import org.hibernate.cfg.Configuration;
import org.hibernate.cfg.Environment;
import org.netxms.reporting.model.ReportResult;
import org.netxms.reporting.services.CommunicationManager;
import org.netxms.reporting.services.ReportManager;
import org.netxms.reporting.tools.SmtpSender;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import net.sf.jasperreports.engine.DefaultJasperReportsContext;
import net.sf.jasperreports.engine.query.QueryExecuterFactory;

/**
 * Server application class
 */
public final class Server implements Daemon
{
   private static Logger logger = LoggerFactory.getLogger(Server.class);

   private ServerSocket serverSocket;
   private Thread listenerThread;
   private SessionFactory sessionFactory;
   private CommunicationManager communicationManager;
   private ReportManager reportManager;
   private Properties configuration;
   private ThreadPoolExecutor threadPool;
   private SmtpSender smtpSender;

   /**
    * @see org.apache.commons.daemon.Daemon#init(org.apache.commons.daemon.DaemonContext)
    */
   @Override
   public void init(DaemonContext context) throws DaemonInitException, Exception
   {
      logger.debug("Initializing server instance");

      configuration = new Properties();
      configuration.putAll(System.getProperties());
      configuration.putAll(loadProperties("nxreportd.properties"));

      threadPool = new ThreadPoolExecutor(8, 128, 600, TimeUnit.SECONDS, new ArrayBlockingQueue<Runnable>(256));

      serverSocket = new ServerSocket(4710);

      String driver = configuration.getProperty("netxms.db.driver");
      String server = configuration.getProperty("netxms.db.server", "");
      String name = configuration.getProperty("netxms.db.name", "netxms");
      String login = configuration.getProperty("netxms.db.login");
      String password = configuration.getProperty("netxms.db.password");
      DatabaseType type = DatabaseType.lookup(driver);
      if (type == null || login == null || password == null)
         throw new DaemonInitException("Missing or invalid database connection configuration");
      
      String url;
      switch(type)
      {
         case INFORMIX:
            url = "jdbc:informix-sqli://" + server + "/" + name;
            break;
         case MARIADB:
            url = "jdbc:mariadb://" + server + "/" + name;
            break;
         case MSSQL:
            url = "jdbc:sqlserver://" + server + ";DatabaseName=" + name;
            break;
         case MYSQL:
            url = "jdbc:mysql://" + server + "/" + name;
            break;
         case ORACLE:
            url = "jdbc:oracle:thin:@" + server + ":1521:" + name;
            break;
         case POSTGRESQL:
            url = "jdbc:postgresql://" + server + "/" + name;
            break;
         default:
            throw new DaemonInitException("Unsupported database type");
      }
      logger.debug("JDBC URL: " + url);

      Properties settings = new Properties();
      settings.put(Environment.DIALECT, type.getDialect());
      settings.put(Environment.DRIVER, type.getDriver());
      settings.put(Environment.URL, url);
      settings.put(Environment.USER, login);
      settings.put(Environment.PASS, password);
      settings.put(Environment.CURRENT_SESSION_CONTEXT_CLASS, "thread");
      Configuration hbConfiguration = new Configuration();
      hbConfiguration.setProperties(settings);
      hbConfiguration.addAnnotatedClass(ReportResult.class);
      sessionFactory = hbConfiguration.buildSessionFactory();

      communicationManager = new CommunicationManager(this);
      reportManager = new ReportManager(this);
      smtpSender = new SmtpSender(this);

      DefaultJasperReportsContext jrContext = DefaultJasperReportsContext.getInstance();
      jrContext.setProperty(QueryExecuterFactory.QUERY_EXECUTER_FACTORY_PREFIX + "nxcl", "org.netxms.reporting.nxcl.NXCLQueryExecutorFactory");
   }

   /**
    * @see org.apache.commons.daemon.Daemon#start()
    */
   @Override
   public void start() throws Exception
   {
      logger.debug("Starting server instance");

      reportManager.deploy();

      listenerThread = new Thread(new Runnable() {
         @Override
         public void run()
         {
            while(!Thread.currentThread().isInterrupted())
            {
               try
               {
                  final Socket socket = serverSocket.accept();
                  logger.debug("Incoming connection");
                  communicationManager.start(socket);
               }
               catch(IOException e)
               {
                  logger.error("Error accepting incoming connection", e);
               }
            }
         }
      }, "Network Listener");
      listenerThread.start();
   }

   /**
    * @see org.apache.commons.daemon.Daemon#stop()
    */
   @Override
   public void stop() throws Exception
   {
      communicationManager.shutdown();
      listenerThread.interrupt();
   }

   /**
    * @see org.apache.commons.daemon.Daemon#destroy()
    */
   @Override
   public void destroy()
   {
      threadPool.shutdownNow();
      try
      {
         serverSocket.close();
      }
      catch(IOException e)
      {
         logger.warn("Exception while closing listening socket", e);
      }
      reportManager = null;
      communicationManager = null;
      smtpSender = null;
      threadPool = null;
   }

   /**
    * Load properties from file without throwing exception
    *
    * @param name property file name
    * @return properties loaded from file or empty properties
    */
   private static Properties loadProperties(String name)
   {
      ClassLoader classLoader = Server.class.getClassLoader();
      Properties properties = new Properties();
      InputStream stream = null;
      try
      {
         stream = classLoader.getResourceAsStream(name);
         properties.load(stream);
      }
      catch(Exception e)
      {
         logger.error("Unable to load " + name, e);
      }
      finally
      {
         if (stream != null)
         {
            try
            {
               stream.close();
            }
            catch(IOException e)
            {
            }
         }
      }
      return properties;
   }

   /**
    * Get server configuration
    * 
    * @return server configuration
    */
   public Properties getConfiguration()
   {
      return configuration;
   }

   /**
    * Get server configuration property
    *
    * @param name property name
    * @param defaultValue default value
    * @return property value or default value
    */
   public String getConfigurationProperty(String name, String defaultValue)
   {
      return configuration.getProperty(name, defaultValue);
   }

   /**
    * Get Hibernate session factory
    * 
    * @return Hibernate session factory
    */
   public SessionFactory getSessionFactory()
   {
      return sessionFactory;
   }

   /**
    * Get SMTP sender
    * 
    * @return SMTP sender
    */
   public SmtpSender getSmtpSender()
   {
      return smtpSender;
   }

   /**
    * @return the communicationManager
    */
   public CommunicationManager getCommunicationManager()
   {
      return communicationManager;
   }

   /**
    * @return the reportManager
    */
   public ReportManager getReportManager()
   {
      return reportManager;
   }

   /**
    * Execute background task
    *
    * @param task task to be executed
    */
   public void executeBackgroundTask(Runnable task)
   {
      if (threadPool != null)
         threadPool.execute(task);
   }
}
