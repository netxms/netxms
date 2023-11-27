/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.base.windows;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Tray;
import org.eclipse.swt.widgets.TrayItem;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Manager for tray icon
 */
public final class TrayIconManager
{
   private static TrayItem trayIcon = null;

   /**
    * 
    */
   private TrayIconManager()
   {
   }

   /**
    * Show tray icon
    */
   public static void showTrayIcon()
   {
      if (trayIcon != null)
         return; // Tray icon already exist

      Tray tray = Display.getDefault().getSystemTray();
      if (tray != null)
      {
         TrayItem item = new TrayItem(tray, SWT.NONE);
         item.setToolTipText(LocalizationHelper.getI18n(TrayIconManager.class).tr("NetXMS Management Client"));
         item.setImage(ResourceManager.getImage("icons/tray-icon.png"));
         item.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetDefaultSelected(SelectionEvent e)
            {
               final Shell shell = Registry.getMainWindow().getShell();
               shell.setVisible(true);
               shell.setMinimized(false);
            }
         });
         item.addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               item.getImage().dispose();
            }
         });
         trayIcon = item;
      }
   }

   /**
    * Hide tray icon
    */
   public static void hideTrayIcon()
   {
      if (trayIcon == null)
         return; // No tray icon

      trayIcon.dispose();
      trayIcon = null;
   }

   /**
    * Get tray icon.
    *
    * @return tray icon or null
    */
   public static TrayItem getTrayIcon()
   {
      return trayIcon;
   }
}
