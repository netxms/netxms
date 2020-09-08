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
import org.eclipse.swt.widgets.TrayItem;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.base.views.ConfigurationPerspective;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.PinboardPerspective;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.modules.alarms.AlarmsPerspective;
import org.netxms.nxmc.modules.objects.ObjectsPerspective;

/**
 * Global registry
 */
public final class Registry
{
   private static Registry instance = new Registry();
   
   /**
    * Get registry instance
    *
    * @return registry instance
    */
   public static Registry getInstance()
   {
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
    * Get application's main window.
    * 
    * @return application's main window
    */
   public static MainWindow getMainWindow()
   {
      return getInstance().mainWindow;
   }

   private Set<Perspective> perspectives = new HashSet<Perspective>();
   private NXCSession session = null;
   private TimeZone timeZone = null;
   private File stateDir = null;
   private MainWindow mainWindow = null;
   private TrayItem trayIcon = null;

   /**
    * Default constructor
    */
   private Registry()
   {
      perspectives.add(new AlarmsPerspective());
      perspectives.add(new ObjectsPerspective());
      perspectives.add(new ConfigurationPerspective());
      perspectives.add(new PinboardPerspective());
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

   /**
    * Get current tray icon
    *
    * @return current tray icon or null
    */
   public TrayItem getTrayIcon()
   {
      return trayIcon;
   }

   /**
    * Set current tray icon.
    * 
    * @param trayIcon new tray icon
    */
   public void setTrayIcon(TrayItem trayIcon)
   {
      this.trayIcon = trayIcon;
   }
}
