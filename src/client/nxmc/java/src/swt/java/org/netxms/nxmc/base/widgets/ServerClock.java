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
package org.netxms.nxmc.base.widgets;

import java.text.DateFormat;
import java.util.Date;
import java.util.TimeZone;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Widget showing server clock
 */
public class ServerClock extends Composite
{
   private static final int REFRESH_TIME = 30000;

   private I18n i18n = LocalizationHelper.getI18n(ServerClock.class);
   private Label time;
   private boolean showText;
   private boolean showTimeZone;
   private Runnable displayFormatChangeListener;

   /**
    * @param parent
    * @param style
    */
   public ServerClock(Composite parent, int style)
   {
      super(parent, style);
    
      setLayout(new FillLayout());
      
      time = new Label(this, SWT.NONE);
      time.setToolTipText(i18n.tr("Server time"));

      FontData fd = time.getFont().getFontData()[0];
      fd.setStyle(SWT.BOLD);
      final Font font = new Font(time.getDisplay(), fd);
      time.setFont(font);
      time.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            font.dispose();
         }
      });

      createContextMenu();

      final Runnable timer = new Runnable() {
         public void run() {
            if (time.isDisposed())
               return;
            time.getDisplay().timerExec(REFRESH_TIME, this);
            updateTimeDisplay();
         }
      };
      timer.run();
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      final PreferenceStore settings = PreferenceStore.getInstance();

      showText = settings.getAsBoolean("ServerClock.showText", false);
      showTimeZone = settings.getAsBoolean("ServerClock.showTimeZone", false);

      final Action actionShowText = new Action(i18n.tr("Show &text"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            showText = isChecked();
            settings.set("ServerClock.showText", showText);
            updateTimeDisplay();
            if (displayFormatChangeListener != null)
               displayFormatChangeListener.run();
         }
      };
      actionShowText.setChecked(showText);

      final Action actionShowTimeZone = new Action(i18n.tr("Show time &zone"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            showTimeZone = isChecked();
            settings.set("ServerClock.showTimeZone", showTimeZone);
            updateTimeDisplay();
            if (displayFormatChangeListener != null)
               displayFormatChangeListener.run();
         }
      };
      actionShowTimeZone.setChecked(showTimeZone);

      final MenuManager menuManager = new MenuManager();
      menuManager.add(actionShowText);
      menuManager.add(actionShowTimeZone);
      setMenu(menuManager.createContextMenu(this));
      time.setMenu(menuManager.createContextMenu(time));
   }

   /**
    * Update time displayed
    */
   private void updateTimeDisplay()
   {
      DateFormat df = DateFormatFactory.getShortTimeFormat();
      NXCSession session = Registry.getSession();
      String tz = session.getServerTimeZone();
      df.setTimeZone(TimeZone.getTimeZone(tz.replaceAll("[A-Za-z]+([\\+\\-][0-9]+)(:[0-9]+)?.*", "GMT$1$2")));
      String timeText = showTimeZone ? (df.format(new Date(session.getServerTime())) + " " + tz) : df.format(new Date(session.getServerTime()));
      time.setText(showText ? String.format(i18n.tr("Server time is %s"), timeText) : timeText);
      getParent().layout(true, true);
   }

   /**
    * @return the displayFormatChangeListener
    */
   public Runnable getDisplayFormatChangeListener()
   {
      return displayFormatChangeListener;
   }

   /**
    * @param displayFormatChangeListener the displayFormatChangeListener to set
    */
   public void setDisplayFormatChangeListener(Runnable displayFormatChangeListener)
   {
      this.displayFormatChangeListener = displayFormatChangeListener;
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setBackground(org.eclipse.swt.graphics.Color)
    */
   @Override
   public void setBackground(Color color)
   {
      super.setBackground(color);
      time.setBackground(color);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setForeground(org.eclipse.swt.graphics.Color)
    */
   @Override
   public void setForeground(Color color)
   {
      super.setForeground(color);
      time.setForeground(color);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setFont(org.eclipse.swt.graphics.Font)
    */
   @Override
   public void setFont(Font font)
   {
      super.setFont(font);
      time.setFont(font);
   }
}
