package org.netxms;

import org.netxms.api.client.NetXMSClientException;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCSession;
import org.python.core.Py;
import org.python.core.PyFile;
import org.python.core.PySystemState;
import org.python.core.imp;
import org.python.util.InteractiveConsole;
import org.python.util.JLineConsole;
import java.io.Console;
import java.io.IOException;
import java.io.File;
import java.util.Properties;

public class Shell
{
   public static final String DEFAULT_SERVER = "127.0.0.1";
   public static final String DEFAULT_LOGIN = "admin";
   public static final String DEFAULT_PASSWORD = "netxms";
   private String server;
   private String login;
   private String password;

   /**
    * @param args
    */
   public static void main(String[] args)
   {
      final Shell shell = new Shell();
      try
      {
         shell.run(args);
      }
      catch(org.python.core.PyException e)
      {
         e.printStackTrace();
      }
      catch(Exception e)
      {
         System.out.println(e.getClass().getName() + ": " + e.getMessage());
      }
   }

   /**
    * @param args
    * @throws IOException
    * @throws NetXMSClientException
    */
   private void run(String[] args) throws IOException, NetXMSClientException
   {
      initJython(args);

      readCredentials(args.length == 0 && isInteractive());
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
   private void readCredentials(boolean interactive)
   {
      server = System.getProperty("netxms.server");
      login = System.getProperty("netxms.login");
      password = System.getProperty("netxms.password");

      if (interactive)
      {
         final Console console = System.console();
         if (server == null)
         {
            server = console.readLine("Server IP [127.0.0.1]: ");
         }
         if (login == null)
         {
            login = console.readLine("Login [admin]: ");
         }
         if (password == null)
         {
            final char[] passwordChars = console.readPassword("Password [netxms]: ");
            if (passwordChars == null || passwordChars.length == 0)
            {
               password = DEFAULT_PASSWORD;
            }
            else
            {
               password = new String(passwordChars);
            }
         }
      }
      if (server == null || server.length() == 0)
      {
         server = DEFAULT_SERVER;
      }
      if (login == null || login.length() == 0)
      {
         login = DEFAULT_LOGIN;
      }
      if (password == null)
      {
         password = DEFAULT_PASSWORD;
      }
   }

   /**
    * @return
    */
   private String getBanner()
   {
      return "NetXMS " + NXCommon.VERSION + " Interactive Shell";
   }

   /**
    * @return
    * @throws IOException
    * @throws NetXMSClientException
    */
   private NXCSession connect() throws IOException, NetXMSClientException
   {
      boolean encrypt = true;
      String encryptOption = System.getProperty("netxms.encryptSession");
      if (encryptOption != null)
      {
         encrypt = Boolean.parseBoolean(encryptOption);
      }
      
      final NXCSession session = new NXCSession(server, NXCSession.DEFAULT_CONN_PORT, login, password, encrypt);
      session.connect();
      session.syncObjects();
      session.syncUserDatabase();
      return session;
   }

   /**
    * @return
    */
   protected boolean isInteractive()
   {
      PySystemState systemState = Py.getSystemState();
      boolean interactive = ((PyFile)Py.defaultSystemState.stdin).isatty();
      if (!interactive)
      {
         systemState.ps1 = systemState.ps2 = Py.EmptyString;
      }
      return interactive;
   }

   /**
    * @param args
    * @return
    */
   private InteractiveConsole createInterpreter(String args[])
   {
      final InteractiveConsole console;
      if (isInteractive())
      {
         console = new JLineConsole();
      }
      else
      {
         console = new InteractiveConsole();
      }
      Py.getSystemState().__setattr__("_jy_interpreter", Py.java2py(console));
      imp.load("site");

      console.exec("from org.netxms.client import *");
      console.exec("from org.netxms.api.client import *");

      console.set("__name__", "__nxshell__");

      return console;
   }

   /**
    * @param args
    */
   private void initJython(String[] args)
   {
      final Properties postProperties = new Properties();
      final File tempDirectory = new File(System.getProperty("java.io.tmpdir"));
      final File cacheDir = new File(tempDirectory, "nxshell");
      postProperties.setProperty("python.cachedir", cacheDir.getPath());
      postProperties.setProperty("python.cachedir.skip", "false");
      PySystemState.initialize(PySystemState.getBaseProperties(), postProperties, args);
   }
}
