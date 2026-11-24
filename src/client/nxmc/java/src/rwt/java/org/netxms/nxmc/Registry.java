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
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.base.views.ConfigurationPerspective;
import org.netxms.nxmc.base.views.MonitorPerspective;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PinLocation;
import org.netxms.nxmc.base.views.PinboardPerspective;
import org.netxms.nxmc.base.views.ToolsPerspective;
import org.netxms.nxmc.modules.ai.AiPerspective;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.modules.alarms.AlarmsPerspective;
import org.netxms.nxmc.modules.assetmanagement.AssetsPerspective;
import org.netxms.nxmc.modules.incidents.IncidentsPerspective;
import org.netxms.nxmc.modules.businessservice.BusinessServicesPerspective;
import org.netxms.nxmc.modules.dashboards.DashboardsPerspective;
import org.netxms.nxmc.modules.datacollection.GraphsPerspective;
import org.netxms.nxmc.modules.logviewer.views.LogViewerPerspective;
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
   public static final boolean IS_WEB_CLIENT = true;

   /**
    * Get registry instance.
    *
    * @return registry instance
    */
   private static Registry getInstance()
   {
      return getInstance(Display.getCurrent());
   }

   /**
    * Get registry instance.
    *
    * @param display display to use for registry access
    * @return registry instance
    */
   private static Registry getInstance(Display display)
   {
      Registry instance = (Registry)RWT.getUISession(display).getAttribute("netxms.registry");
      if (instance == null)
      {
         instance = new Registry();
         RWT.getUISession(display).setAttribute("netxms.registry", instance);
      }
      return instance;
   }
   
   /**
    * Make registry of destination display use client session of source display. Destination registry gets reference to client
    * session, poll manager, named properties, and singletons registered with <code>setSingleton</code>; singletons registered
    * with <code>setDisplaySingleton</code> and everything else stay separate for each display. Intended for pop-out windows,
    * which have own display and UI session but work within client session of the window they were opened from.
    *
    * @param src source display
    * @param dst destination display
    */
   public static void copyToDisplay(Display src, Display dst)
   {
      Registry s = getInstance(src);
      Registry d = getInstance(dst);
      d.session = s.session;
      d.pollManager = s.pollManager;
      d.singletonStorage = s.singletonStorage;
      d.storage = s.storage;
      d.sessionOwner = false;
   }

   /**
    * Dispose acquired resources. Singletons shared with pop-out windows are disposed only by registry that owns client session.
    */
   public static void dispose()
   {
      Registry r = getInstance();
      disposeSingletons(r.displaySingletonStorage);
      if (r.sessionOwner)
         disposeSingletons(r.singletonStorage);

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
    * Dispose all singletons in given storage. Storage is not cleared, so that widgets disposed later (including those in pop-out
    * windows that are yet to be closed) get disposed singleton and not null or newly created instance.
    *
    * @param storage singleton storage
    */
   @SuppressWarnings("rawtypes")
   private static void disposeSingletons(Map<Class, Object> storage)
   {
      synchronized(storage)
      {
         for(Object s : storage.values())
         {
            if (s instanceof DisposableSingleton)
               ((DisposableSingleton)s).dispose();
         }
      }
   }

   /**
    * Get current NetXMS client library session
    * 
    * @return Current session
    */
   public static NXCSession getSession()
   {
      return getInstance().session;
   }

   /**
    * Get current NetXMS client library session
    *
    * @param display display to use
    * @return Current session
    */
   public static NXCSession getSession(Display display)
   {
      return getInstance(display).session;
   }

   /**
    * Get client timezone
    * 
    * @return
    */
   public static TimeZone getTimeZone()
   {
      return getInstance().timeZone;
   }

   /**
    * Get application's state directory.
    * 
    * @return application's state directory
    */
   public static File getStateDir()
   {
      return getInstance().stateDir;
   }

   /**
    * Get application's state directory.
    * 
    * @param display display to use
    * @return application's state directory
    */
   public static File getStateDir(Display display)
   {
      return getInstance(display).stateDir;
   }

   /**
    * Get application's main window.
    *
    * @return application's main window
    */
   public static MainWindow getMainWindow()
   {
      return getInstance().mainWindow;
   }

   /**
    * Get application's main window shell.
    *
    * @return application's main window shell or null
    */
   public static Shell getMainWindowShell()
   {
      MainWindow w = getMainWindow();
      return (w != null) ? w.getShell() : null;
   }

   @SuppressWarnings("rawtypes")
   private Map<Class, Object> singletonStorage = new HashMap<Class, Object>(); // Shared with pop-out windows
   @SuppressWarnings("rawtypes")
   private Map<Class, Object> displaySingletonStorage = new HashMap<Class, Object>();
   private boolean sessionOwner = true;
   private Map<String, Object> storage = new HashMap<String, Object>(); // Shared with pop-out windows

   /**
    * Find singleton object of given class among singletons bound to display of this registry, then among shared ones.
    *
    * @param singletonClass class of singleton object
    * @return singleton object or null
    */
   @SuppressWarnings("unchecked")
   private <T> T findSingleton(Class<T> singletonClass)
   {
      synchronized(displaySingletonStorage)
      {
         T singleton = (T)displaySingletonStorage.get(singletonClass);
         if (singleton != null)
            return singleton;
      }
      synchronized(singletonStorage)
      {
         return (T)singletonStorage.get(singletonClass);
      }
   }

   /**
    * Get singleton object of given class.
    *
    * @param singletonClass class of singleton object
    * @return singleton object or null
    */
   public static <T> T getSingleton(Class<T> singletonClass)
   {
      return getInstance().findSingleton(singletonClass);
   }

   /**
    * Get singleton object of given class.
    *
    * @param singletonClass class of singleton object
    * @param display display object
    * @return singleton object or null
    */
   public static <T> T getSingleton(Class<T> singletonClass, Display display)
   {
      return getInstance(display).findSingleton(singletonClass);
   }

   /**
    * Set singleton object of given class. In web client such singleton is shared by main window and all pop-out windows opened
    * from it, so it can be accessed from UI threads of different windows and should not hold resources bound to display - use
    * <code>setDisplaySingleton</code> for those.
    *
    * @param singletonClass class of singleton object
    * @param singleton singleton object
    */
   public static void setSingleton(Class<?> singletonClass, Object singleton)
   {
      Registry r = getInstance();      
      synchronized(r.singletonStorage)
      {
         r.singletonStorage.put(singletonClass, singleton);
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
      Registry r = getInstance(display);      
      synchronized(r.singletonStorage)
      {
         r.singletonStorage.put(singletonClass, singleton);
      }
   }

   /**
    * Set singleton object of given class that is bound to given display (holds images or other display resources, or otherwise
    * cannot be used from another display). Each pop-out window has own display and so own instance of such singleton.
    *
    * @param display display
    * @param singletonClass class of singleton object
    * @param singleton singleton object
    */
   public static void setDisplaySingleton(Display display, Class<?> singletonClass, Object singleton)
   {
      Registry r = getInstance(display);
      synchronized(r.displaySingletonStorage)
      {
         r.displaySingletonStorage.put(singletonClass, singleton);
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
      Registry r = getInstance();  
      synchronized(r.storage)
      {
         return r.storage.get("netxms." + name);
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
      Registry r = getInstance(display);  
      synchronized(r.storage)
      {
         return r.storage.get("netxms." + name);
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
      Registry r = getInstance();  
      synchronized(r.storage)
      {
         r.storage.put("netxms." + name, value);
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
      Registry r = getInstance(display);  
      synchronized(r.storage)
      {
         r.storage.put("netxms." + name, value);
      }
   }

   /**
    * Get application's main window.
    * 
    * @return application's main window
    */
   public static PollManager getPollManager()
   {
      return getInstance().pollManager;
   }

   private NXCSession session = null;
   private TimeZone timeZone = null;
   private File stateDir = null;
   private MainWindow mainWindow = null;
   private PollManager pollManager = null;
   private PinLocation lastViewPinLocation = PinLocation.PINBOARD;
   private Set<Perspective> perspectives = new HashSet<Perspective>();

   /**
    * Default constructor
    */
   private Registry()
   {
      pollManager = new PollManager();

      perspectives.add(new AlarmsPerspective());
      perspectives.add(new IncidentsPerspective());
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
      perspectives.add(new AiPerspective());

      ServiceLoader<PerspectiveDescriptor> loader = ServiceLoader.load(PerspectiveDescriptor.class, Registry.class.getClassLoader());
      for(PerspectiveDescriptor p : loader)
      {
         perspectives.add(p.createPerspective());
      }
   }

   /**
    * Get registered perspectives ordered by priority
    *
    * @return the perspectives
    */
   public static List<Perspective> getPerspectives()
   {
      List<Perspective> result = new ArrayList<Perspective>(getInstance().perspectives);
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
    * @param display current display
    * @param session current session
    */
   public static void setSession(Display display, NXCSession session)
   {
      getInstance(display).session = session;
   }

   /**
    * Set server time zone
    */
   public static void setServerTimeZone()
   {
      Registry instance = getInstance();
      if (instance.session != null)
      {
         String tz = instance.session.getServerTimeZone();
         instance.timeZone = TimeZone.getTimeZone(tz.replaceAll("[A-Za-z]*([\\+\\-][0-9]+)(:[0-9]+)?.*", "GMT$1$2"));
      }
   }

   /**
    * Reset time zone to default
    */
   public static void resetTimeZone()
   {
      getInstance().timeZone = null;
   }

   /**
    * Set application state directory (should be called only by startup code).
    *
    * @param stateDir application state directory
    */
   protected static void setStateDir(File stateDir)
   {
      Registry instance = getInstance();
      if (instance.stateDir == null)
         instance.stateDir = stateDir;
   }

   /**
    * Set application's main window (should be called only by startup code).
    *
    * @param window application's main window
    */
   protected static void setMainWindow(MainWindow window)
   {
      Registry instance = getInstance();
      if (instance.mainWindow == null)
         instance.mainWindow = window;
   }

   /**
    * Get last used view pin location
    *
    * @return last used view pin location
    */
   public static PinLocation getLastViewPinLocation()
   {
      return getInstance().lastViewPinLocation;
   }

   /**
    * Set last used view pin location
    * 
    * @param lastViewPinLocation last used view pin location
    */
   public static void setLastViewPinLocation(PinLocation lastViewPinLocation)
   {
      getInstance().lastViewPinLocation = lastViewPinLocation;
   }

   /**
    * Get IP address of a client. Always returns null in desktop version.
    *
    * @return IP address of a client or null
    */
   public static String getClientAddress()
   {
      return RWT.getRequest().getRemoteAddr();
   }
}
