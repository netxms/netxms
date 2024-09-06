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
package org.netxms.nxmc;

import java.io.File;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.ServiceLoader;
import java.util.Set;
import java.util.TimeZone;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.base.views.ConfigurationPerspective;
import org.netxms.nxmc.base.views.MonitorPerspective;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PerspectiveSeparator;
import org.netxms.nxmc.base.views.PinLocation;
import org.netxms.nxmc.base.views.PinboardPerspective;
import org.netxms.nxmc.base.views.ToolsPerspective;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.modules.alarms.AlarmsPerspective;
import org.netxms.nxmc.modules.assetmanagement.AssetsPerspective;
import org.netxms.nxmc.modules.businessservice.BusinessServicesPerspective;
import org.netxms.nxmc.modules.datacollection.GraphsPerspective;
import org.netxms.nxmc.modules.logviewer.views.LogViewerPerspective;
import org.netxms.nxmc.modules.objects.DashboardsPerspective;
import org.netxms.nxmc.modules.objects.InfrastructurePerspective;
import org.netxms.nxmc.modules.objects.MapsPerspective;
import org.netxms.nxmc.modules.objects.NetworkPerspective;
import org.netxms.nxmc.modules.objects.TemplatesPerspective;
import org.netxms.nxmc.modules.objects.views.helpers.PollManager;
import org.netxms.nxmc.modules.reporting.ReportingPerspective;
import org.netxms.nxmc.services.PerspectiveDescriptor;

/**
 * Global registry
 */
public final class Registry
{
   public static final boolean IS_WEB_CLIENT = false;

   private static Set<Perspective> perspectives = new HashSet<Perspective>();
   private static NXCSession session = null;
   private static TimeZone timeZone = null;
   private static File stateDir = null;
   private static MainWindow mainWindow = null;
   private static PollManager pollManager = new PollManager();
   private static PinLocation lastViewPinLocation = PinLocation.PINBOARD;
   private static Map<Class<?>, Object> singletons = new HashMap<>();
   private static Map<String, Object> properties = new HashMap<>();
   private static List<View> popOutViews = new ArrayList<View>();

   static
   {
      perspectives.add(new AlarmsPerspective());
      perspectives.add(new AssetsPerspective());
      perspectives.add(new BusinessServicesPerspective());
      perspectives.add(new ConfigurationPerspective());
      perspectives.add(new DashboardsPerspective());
      perspectives.add(new GraphsPerspective());
      perspectives.add(new InfrastructurePerspective());
      perspectives.add(new LogViewerPerspective());
      perspectives.add(new MapsPerspective());
      perspectives.add(new MonitorPerspective());
      perspectives.add(new NetworkPerspective());
      perspectives.add(new PinboardPerspective());
      perspectives.add(new ReportingPerspective());
      perspectives.add(new TemplatesPerspective());
      perspectives.add(new ToolsPerspective());

      perspectives.add(new PerspectiveSeparator(19));
      perspectives.add(new PerspectiveSeparator(29));
      perspectives.add(new PerspectiveSeparator(99));
      perspectives.add(new PerspectiveSeparator(149));
      perspectives.add(new PerspectiveSeparator(254));

      ServiceLoader<PerspectiveDescriptor> loader = ServiceLoader.load(PerspectiveDescriptor.class, Registry.class.getClassLoader());
      for(PerspectiveDescriptor p : loader)
      {
         perspectives.add(p.createPerspective());
      }
   }

   /**
    * Dispose acquired resources
    */
   public static void dispose()
   {
      synchronized(singletons)
      {
         for(Object s : singletons.values())
         {
            if (s instanceof DisposableSingleton)
               ((DisposableSingleton)s).dispose();
         }
      }

      // give a chance to stop gracefully fo threads initiated by registered singletons
      try
      {
         Thread.sleep(100);
      }
      catch(InterruptedException e)
      {
      }
   }

   /**
    * Get current NetXMS client library session
    * 
    * @return Current session
    */
   public static NXCSession getSession()
   {
      return session;
   }

   /**
    * Get current NetXMS client library session
    *
    * @param display display to use
    * @return Current session
    */
   public static NXCSession getSession(Display display)
   {
      return session;
   }

   /**
    * Get client timezone
    * 
    * @return
    */
   public static TimeZone getTimeZone()
   {
      return timeZone;
   }

   /**
    * Get application's state directory.
    * 
    * @return application's state directory
    */
   public static File getStateDir()
   {
      return stateDir;
   }

   /**
    * Get application's state directory.
    * 
    * @param display display to use
    * @return application's state directory
    */
   public static File getStateDir(Display display)
   {
      return stateDir;
   }

   /**
    * Get application's main window.
    *
    * @return application's main window
    */
   public static MainWindow getMainWindow()
   {
      return mainWindow;
   }

   /**
    * Get application's main window shell.
    *
    * @return application's main window shell or null
    */
   public static Shell getMainWindowShell()
   {
      return (mainWindow != null) ? mainWindow.getShell() : null;
   }

   /**
    * Get singleton object of given class.
    *
    * @param singletonClass class of singleton object
    * @return singleton object or null
    */
   @SuppressWarnings("unchecked")
   public static <T> T getSingleton(Class<T> singletonClass)
   {
      synchronized(singletons)
      {
         return (T)singletons.get(singletonClass);
      }
   }

   /**
    * Get singleton object of given class.
    *
    * @param singletonClass class of singleton object
    * @param display display object
    * @return singleton object or null
    */
   @SuppressWarnings("unchecked")
   public static <T> T getSingleton(Class<T> singletonClass, Display display)
   {
      synchronized(singletons)
      {
         return (T)singletons.get(singletonClass);
      }
   }

   /**
    * Set singleton object of given class.
    *
    * @param singletonClass class of singleton object
    * @param singleton singleton object
    */
   public static void setSingleton(Class<?> singletonClass, Object singleton)
   {
      synchronized(singletons)
      {
         singletons.put(singletonClass, singleton);
      }
   }

   /**
    * Set singleton object of given class using specific display.
    *
    * @param display display
    * @param singletonClass class of singleton object
    * @param singleton singleton object
    */
   public static void setSingleton(Display display, Class<?> singletonClass, Object singleton)
   {
      synchronized(singletons)
      {
         singletons.put(singletonClass, singleton);
      }
   }

   /**
    * Get named property.
    *
    * @param name property name
    * @return value of property or null
    */
   public static Object getProperty(String name)
   {
      synchronized(properties)
      {
         return properties.get(name);
      }
   }

   /**
    * Get named property using given display.
    *
    * @param name property name
    * @param display display to use
    * @return value of property or null
    */
   public static Object getProperty(String name, Display display)
   {
      synchronized(properties)
      {
         return properties.get(name);
      }
   }

   /**
    * Get value of console property as boolean
    * 
    * @param name name of the property
    * @param defaultValue default value if property does not exist or is not boolean
    * @return property value or default value
    */
   public static boolean getPropertyAsBoolean(final String name, boolean defaultValue)
   {
      Object v = getProperty(name);
      return ((v != null) && (v instanceof Boolean)) ? (Boolean)v : defaultValue;
   }

   /**
    * Get value of console property as integer
    * 
    * @param name name of the property
    * @param defaultValue default value if property does not exist or is not boolean
    * @return property value or default value
    */
   public static int getPropertyAsInteger(final String name, int defaultValue)
   {
      Object v = getProperty(name);
      return ((v != null) && (v instanceof Integer)) ? (Integer)v : defaultValue;
   }

   /**
    * Set named property.
    *
    * @param name property name
    * @param value property vakue
    */
   public static void setProperty(String name, Object value)
   {
      synchronized(properties)
      {
         properties.put(name, value);
      }
   }

   /**
    * Set named property.
    *
    * @param name property name
    * @param value property vakue
    */
   public static void setProperty(Display display, String name, Object value)
   {
      synchronized(properties)
      {
         properties.put(name, value);
      }
   }

   /**
    * Get poll manager
    * 
    * @return poll manager
    */
   public static PollManager getPollManager()
   {
      return pollManager;
   }

   /**
    * Get registered perspectives ordered by priority
    *
    * @return the perspectives
    */
   public static List<Perspective> getPerspectives()
   {
      List<Perspective> result = new ArrayList<Perspective>(perspectives);
      result.sort(new Comparator<Perspective>() {
         @Override
         public int compare(Perspective p1, Perspective p2)
         {
            return p1.getPriority() - p2.getPriority();
         }
      });
      return result;
   }

   /**
    * Set current NetXMS client library session
    * 
    * @param session Current session
    */
   public static void setSession(Display display, NXCSession session)
   {
      Registry.session = session;
   }

   /**
    * Set server time zone
    */
   public static void setServerTimeZone()
   {
      if (session != null)
      {
         String tz = session.getServerTimeZone();
         timeZone = TimeZone.getTimeZone(tz.replaceAll("[A-Za-z]+([\\+\\-][0-9]+).*", "GMT$1"));
      }
   }

   /**
    * Reset time zone to default
    */
   public static void resetTimeZone()
   {
      timeZone = null;
   }

   /**
    * Set application state directory (should be called only by startup code).
    *
    * @param stateDir application state directory
    */
   protected static void setStateDir(File stateDir)
   {
      if (Registry.stateDir == null)
         Registry.stateDir = stateDir;
   }

   /**
    * Set application's main window (should be called only by startup code).
    *
    * @param window application's main window
    */
   protected static void setMainWindow(MainWindow window)
   {
      if (mainWindow == null)
         mainWindow = window;
   }

   /**
    * Get last used view pin location
    *
    * @return last used view pin location
    */
   public static PinLocation getLastViewPinLocation()
   {
      return lastViewPinLocation;
   }

   /**
    * Set last used view pin location
    * 
    * @param lastViewPinLocation last used view pin location
    */
   public static void setLastViewPinLocation(PinLocation lastViewPinLocation)
   {
      Registry.lastViewPinLocation = lastViewPinLocation;
   }

   /**
    * Get IP address of a client. Always returns null in desktop version.
    *
    * @return IP address of a client or null
    */
   public static String getClientAddress()
   {
      return null;
   }

   /**
    * Get registered pop-out views 
    * 
    * @return list of pop-out views 
    */
   public static List<View> getPopoutViews()
   {
      synchronized(popOutViews)
      {
         return popOutViews;
      }
   }
   
   /**
    * Register opened view
    * 
    * @param view view to register
    */
   public static void registerPopoutView(View view)
   {
      synchronized(popOutViews)
      {
         popOutViews.add(view);
      }
   }
   
   /**
    * Remove registered view
    * 
    * @param view view to remove
    */
   public static void unregisterPopoutView(View view)
   {
      synchronized(popOutViews)
      {
         popOutViews.remove(view);
      }
   }
}
