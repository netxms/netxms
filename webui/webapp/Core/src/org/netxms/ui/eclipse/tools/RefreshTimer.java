/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.tools;

import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import org.eclipse.swt.widgets.Control;

/**
 * Refresh timer helper class - calls given runnable on demand,
 * but not often then configured time period.
 */
public class RefreshTimer
{
   private static ScheduledThreadPoolExecutor executor = new ScheduledThreadPoolExecutor(4);
   
   private Control control;
   private Runnable callback;
   private long interval;
   private long lastRun = 0;
   private boolean scheduled = false;
   private boolean refreshDisabled = false;
   private boolean updateMissed = false;
   
   /**
    * Create new refresh timer
    * 
    * @param interval minimal interval between executions (in milliseconds)
    * @param control control to check for disposal
    * @param callback callback to run
    */
   public RefreshTimer(int interval, Control control, Runnable callback)
   {
      this.interval = interval;
      this.control = control;
      this.callback = callback;
   }
   
   /**
    * Execute runnable if possible. If last execution was later than current time minus
    * interval, runnable execution will be delayed. Multiple delayed execution requests will
    * be reduced into one.
    */
   public synchronized void execute()
   {
      if(refreshDisabled)
      {
         updateMissed = true;
         return;
      }
      
      if (control.isDisposed())
         return;
      
      long curr = System.currentTimeMillis();
      if ((interval <= 0) || (curr - lastRun >= interval))
      {
         lastRun = curr;
         control.getDisplay().asyncExec(callback);
      }
      else if (!scheduled)
      {
         final int delay = (int)(interval - (curr - lastRun));
         scheduled = true;
         executor.schedule(new Runnable() {
            @Override
            public void run()
            {

               if(refreshDisabled)
               {
                  updateMissed = true;
                  return;
               }
               
               control.getDisplay().asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     onTimer();
                     if (!control.isDisposed())
                        callback.run();
                  }
               });
            }
         }, delay, TimeUnit.MILLISECONDS);
      }
   }
   
   /**
    * Update state after scheduled runnable finishes
    */
   private synchronized void onTimer()
   {
      lastRun = System.currentTimeMillis();
      scheduled = false;
   }
   
   /**
    * Disables refresh 
    */
   public synchronized void disableRefresh()
   {
      refreshDisabled = true;
   }
   
   /**
    * Enables refresh
    */
   public synchronized void enableRefresh()
   {
      refreshDisabled = false;
      if(updateMissed)
         execute();
   }
}
