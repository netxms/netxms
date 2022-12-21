/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.TimeZone;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.base.views.ConfigurationPerspective;
import org.netxms.nxmc.base.views.MonitorPerspective;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PinboardPerspective;
import org.netxms.nxmc.base.views.ToolsPerspective;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.modules.alarms.AlarmsPerspective;
import org.netxms.nxmc.modules.businessservice.BusinessServicesPerspective;
import org.netxms.nxmc.modules.logviewer.views.LogViewerPerspective;
import org.netxms.nxmc.modules.objects.DashboardsPerspective;
import org.netxms.nxmc.modules.objects.InfrastructurePerspective;
import org.netxms.nxmc.modules.objects.MapsPerspective;
import org.netxms.nxmc.modules.objects.NetworkPerspective;
import org.netxms.nxmc.modules.objects.TemplatesPerspective;
import org.netxms.nxmc.modules.worldmap.WorldMapPerspective;

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
   public static Registry getInstance()
   {
      Registry instance = (Registry)RWT.getUISession().getAttribute("netxms.registry");
      if (instance == null)
      {
         instance = new Registry();
         RWT.getUISession().setAttribute("netxms.registry", instance);
      }
      return instance;
   }

   /**
    * Get registry instance.
    *
    * @param display display to use for registry access
    * @return registry instance
    */
   public static Registry getInstance(Display display)
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
    * Get named property.
    *
    * @param name property name
    * @return value of property or null
    */
   public static Object getProperty(String name)
   {
      return RWT.getUISession().getAttribute("netxms." + name);
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
      return RWT.getUISession(display).getAttribute("netxms." + name);
   }

   /**
    * Set named property.
    *
    * @param name property name
    * @param value property vakue
    */
   public static void setProperty(String name, Object value)
   {
      RWT.getUISession().setAttribute("netxms." + name, value);
   }

   /**
    * Set named property.
    *
    * @param name property name
    * @param value property vakue
    */
   public static void setProperty(Display display, String name, Object value)
   {
      RWT.getUISession(display).setAttribute("netxms." + name, value);
   }

   private Set<Perspective> perspectives = new HashSet<Perspective>();
   private NXCSession session = null;
   private TimeZone timeZone = null;
   private File stateDir = null;
   private MainWindow mainWindow = null;

   /**
    * Default constructor
    */
   private Registry()
   {
      perspectives.add(new AlarmsPerspective());
      perspectives.add(new BusinessServicesPerspective());
      perspectives.add(new ConfigurationPerspective());
      perspectives.add(new DashboardsPerspective());
      perspectives.add(new InfrastructurePerspective());
      perspectives.add(new LogViewerPerspective());
      perspectives.add(new MapsPerspective());
      perspectives.add(new MonitorPerspective());
      perspectives.add(new NetworkPerspective());
      perspectives.add(new PinboardPerspective());
      perspectives.add(new TemplatesPerspective());
      perspectives.add(new ToolsPerspective());
      perspectives.add(new WorldMapPerspective());
   }

   /**
    * Get registered perspectives ordered by priority
    *
    * @return the perspectives
    */
   public List<Perspective> getPerspectives()
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
   public void setSession(NXCSession session)
   {
      this.session = session;
   }

   /**
    * Set server time zone
    */
   public void setServerTimeZone()
   {
      if (session != null)
      {
         String tz = session.getServerTimeZone();
         timeZone = TimeZone.getTimeZone(tz.replaceAll("[A-Za-z]+([\\+\\-][0-9]+).*", "GMT$1")); //$NON-NLS-1$ //$NON-NLS-2$
      }
   }

   /**
    * Reset time zone to default
    */
   public void resetTimeZone()
   {
      timeZone = null;
   }

   /**
    * Set application state directory (should be called only by startup code).
    *
    * @param stateDir application state directory
    */
   public void setStateDir(File stateDir)
   {
      if (this.stateDir == null)
         this.stateDir = stateDir;
   }

   /**
    * Set application's main window (should be called only by startup code).
    *
    * @param window application's main window
    */
   public void setMainWindow(MainWindow window)
   {
      if (this.mainWindow == null)
         this.mainWindow = window;
   }
}
