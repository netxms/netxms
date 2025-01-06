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
package org.netxms;

import java.io.Console;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;
import org.netxms.base.VersionInfo;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.python.core.Py;
import org.python.core.PyFile;
import org.python.core.PySystemState;
import org.python.core.imp;
import org.python.util.InteractiveConsole;

/**
 * nxshell launcher
 */
public class Shell
{
   private static final String DEFAULT_SERVER = "127.0.0.1";
   private static final String DEFAULT_LOGIN = "admin";

   private Properties configuration = new Properties();
   private String optServer;
   private String optPort;
   private String optLogin;
   private String optPassword;
   private String optToken;

   /**
    * @param args
    */
   public static void main(String[] args)
   {
      final Shell shell = new Shell();
      try
      {
         shell.configuration.putAll(System.getProperties());
         shell.configuration.putAll(loadProperties("nxshell.properties"));
         String customPropertyFile = shell.configuration.getProperty("nxshell.properties");
         if (customPropertyFile != null)
            shell.configuration.putAll(loadProperties(customPropertyFile));
         shell.run(args);
      }
      catch(Throwable e)
      {
         e.printStackTrace();
      }
   }

   /**
    * @param args
    * @throws IOException
    * @throws NetXMSClientException
    */
   private void run(String[] args) throws IOException, NXCException
   {
      initJython(args);

      if (!isInteractive())
      {
         PySystemState systemState = Py.getSystemState();
         systemState.ps1 = systemState.ps2 = Py.EmptyString;
      }

      readCredentials();

      final NXCSession session = connect();

      final InteractiveConsole console = createInterpreter(args);

      console.set("session", session);
      console.set("s", session);

      if (args.length == 0)
      {
         console.interact(getBanner(), null);
      }
      else
      {
         console.execfile(args[0]);
      }
      console.cleanup();

      session.disconnect();
   }

   /**
    * @param interactive
    */
   private void readCredentials()
   {
      optServer = configuration.getProperty("netxms.server");
      optPort = configuration.getProperty("netxms.port");
      optLogin = configuration.getProperty("netxms.login");
      optPassword = configuration.getProperty("netxms.password");
      optToken = configuration.getProperty("netxms.token");

      if (isInteractive())
      {
         final Console console = System.console();
         if (optServer == null)
         {
            optServer = console.readLine("Server IP [127.0.0.1]: ");
            if (optPort == null)
            {
               optPort = console.readLine("Server TCP port [4701]: ");
            }
         }
         if ((optLogin == null) && (optToken == null))
         {
            optLogin = console.readLine("Login [admin]: ");
         }
         if ((optPassword == null) && (optToken == null))
         {
            final char[] passwordChars = console.readPassword("Password: ");
            if (passwordChars == null)
            {
               optPassword = "";
            }
            else
            {
               optPassword = new String(passwordChars);
            }
         }
      }
      if (optServer == null || optServer.length() == 0)
      {
         optServer = DEFAULT_SERVER;
      }
      if (optLogin == null || optLogin.length() == 0)
      {
         optLogin = DEFAULT_LOGIN;
      }
      if (optPassword == null)
      {
         optPassword = "";
      }
   }

   /**
    * @return
    */
   private String getBanner()
   {
      return "NetXMS " + VersionInfo.version() + " Interactive Shell";
   }

   /**
    * @return
    * @throws IOException
    * @throws NetXMSClientException
    */
   private NXCSession connect() throws IOException, NXCException
   {
      boolean enableCompression = true;
      String enableCompressionOption = configuration.getProperty("netxms.enableCompression");
      if (enableCompressionOption != null)
      {
         enableCompression = Boolean.parseBoolean(enableCompressionOption);
      }

      boolean sync = true;
      String syncOption = configuration.getProperty("netxms.syncObjects");
      if (syncOption != null)
      {
         sync = Boolean.parseBoolean(syncOption);
      }

      final String hostName;
      int port = NXCSession.DEFAULT_CONN_PORT;
      if ((optPort != null) && !optPort.isEmpty())
      {
         hostName = optServer;
         try
         {
            port = Integer.valueOf(optPort);
         }
         catch(NumberFormatException e)
         {
            // ignore
         }
      }
      else
      {
         final String[] parts = optServer.split(":");
         if (parts.length == 2)
         {
            hostName = parts[0];
            try
            {
               port = Integer.valueOf(parts[1]);
            }
            catch(NumberFormatException e)
            {
               // ignore
            }
         }
         else
         {
            hostName = optServer;
         }
      }

      final NXCSession session = new NXCSession(hostName, port, enableCompression);
      session.connect();
      if (optToken != null)
         session.login(optToken);
      else
         session.login(optLogin, optPassword);
      if (sync)
      {
         session.syncObjects();
         session.syncUserDatabase();
      }
      return session;
   }

   /**
    * @return
    */
   private boolean isInteractive()
   {
      if (System.getProperties().getProperty("nxshell.interactive") != null)
         return true;
      return ((PyFile)Py.defaultSystemState.stdin).isatty();
   }

   /**
    * @param args
    * @return
    */
   private InteractiveConsole createInterpreter(String args[])
   {
      PySystemState systemState = Py.getSystemState();

      final InteractiveConsole console;
      if (!isInteractive())
      {
         systemState.ps1 = systemState.ps2 = Py.EmptyString;
      }
      console = new InteractiveConsole();
      Py.getSystemState().__setattr__("_jy_interpreter", Py.java2py(console));
      imp.load("site");

      console.exec("from org.netxms.base import *");
      console.exec("from org.netxms.client import *");
      console.exec("from org.netxms.client.agent.config import *");
      console.exec("from org.netxms.client.asset import *");
      console.exec("from org.netxms.client.businessservices import *");
      console.exec("from org.netxms.client.constants import *");
      console.exec("from org.netxms.client.dashboards import *");
      console.exec("from org.netxms.client.datacollection import *");
      console.exec("from org.netxms.client.events import *");
      console.exec("from org.netxms.client.log import *");
      console.exec("from org.netxms.client.maps import *");
      console.exec("from org.netxms.client.maps.configs import *");
      console.exec("from org.netxms.client.maps.elements import *");
      console.exec("from org.netxms.client.mt import *");
      console.exec("from org.netxms.client.objects import *");
      console.exec("from org.netxms.client.objects.configs import *");
      console.exec("from org.netxms.client.objects.interfaces import *");
      console.exec("from org.netxms.client.objects.queries import *");
      console.exec("from org.netxms.client.objecttools import *");
      console.exec("from org.netxms.client.packages import *");
      console.exec("from org.netxms.client.reporting import *");
      console.exec("from org.netxms.client.search import *");
      console.exec("from org.netxms.client.server import *");
      console.exec("from org.netxms.client.services import *");
      console.exec("from org.netxms.client.snmp import *");
      console.exec("from org.netxms.client.topology import *");
      console.exec("from org.netxms.client.users import *");
      console.exec("from org.netxms.client.xml import *");

      console.set("__name__", "__nxshell__");

      return console;
   }

   /**
    * @param args
    */
   private void initJython(String[] args)
   {
      final Properties postProperties = new Properties();
      final File tempDirectory = new File(configuration.getProperty("java.io.tmpdir"));
      final File cacheDir = new File(tempDirectory, "nxshell");
      postProperties.setProperty("python.cachedir", cacheDir.getPath());
      postProperties.setProperty("python.cachedir.skip", "false");
      PySystemState.initialize(PySystemState.getBaseProperties(), postProperties, args);
   }

   /**
    * Load properties from file without throwing exception
    *
    * @param name property file name
    * @return properties loaded from file or empty properties
    */
   private static Properties loadProperties(String name)
   {
      ClassLoader classLoader = Shell.class.getClassLoader();
      Properties properties = new Properties();
      InputStream stream = null;
      try
      {
         stream = classLoader.getResourceAsStream(name);
         if (stream == null)
         {
            File f = new File(name);
            if (f.isFile())
            {
               stream = new FileInputStream(f);
            }
         }
         if (stream != null)
         {
            properties.load(stream);
         }
      }
      catch(Exception e)
      {
         System.err.println("Error loading properties file " + name);
         e.printStackTrace();
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
}
