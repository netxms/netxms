/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.tools;

import org.eclipse.swt.widgets.Display;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewStateListener;

/**
 * Helper class to handle view refresh
 */
public class ViewRefreshController implements ViewStateListener
{
   private Runnable timer;
   private Runnable handler;
   private VisibilityValidator validator;
   private View view;
   private Display display;
   private int interval;
   private boolean disposed = false;
   
   /**
    * Create refresh controller attached to given view.
    * 
    * @param view view to attach to
    * @param interval refresh interval in seconds
    * @param handler refresh handler
    */
   public ViewRefreshController(final View view, int interval, Runnable handler)
   {
      this(view, interval, handler, null);
   }
   
   /**
    * Create refresh controller attached to given view.
    * 
    * @param view to attach to
    * @param interval refresh interval in seconds
    * @param handler refresh handler
    * @param validator custom visibility validator
    */
   public ViewRefreshController(final View view, int interval, Runnable handler, VisibilityValidator validator)
   {
      this.handler = handler;
      this.validator = validator;
      this.interval = interval;
      this.view = view;
      display = view.getDisplay();
      timer = new Runnable() {
         @Override
         public void run()
         {
            if (view.isVisible() &&
                ((ViewRefreshController.this.validator == null) || ViewRefreshController.this.validator.isVisible()))
            {
               ViewRefreshController.this.handler.run();
            }
            display.timerExec((ViewRefreshController.this.interval > 0) ? ViewRefreshController.this.interval * 1000 : -1, this);
         }
      };
      view.addStateListener(this);
      if (interval > 0)
      {
         display.timerExec(interval * 1000, timer);
      }
   }

   /**
    * Set refresh interval
    * 
    * @param interval refresh interval in seconds
    */
   public void setInterval(int interval)
   {
      this.interval = interval;
      display.timerExec(-1, timer);
      if (interval > 0)
         display.timerExec(interval * 1000, timer);
   }

   /**
    * Dispose controller
    */
   public void dispose()
   {
      if (disposed)
         return;

      view.removeStateListener(this);
      display.timerExec(-1, timer);
      disposed = true;
   }
   
   /**
    * @see org.netxms.nxmc.base.views.ViewStateListener#viewActivated(org.netxms.nxmc.base.views.View)
    */
   @Override
   public void viewActivated(View view)
   {
      if ((interval > 0) && ((ViewRefreshController.this.validator == null) || ViewRefreshController.this.validator.isVisible()))
      {
         handler.run();
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewStateListener#viewDeactivated(org.netxms.nxmc.base.views.View)
    */
   @Override
   public void viewDeactivated(View view)
   {
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewStateListener#viewClosed(org.netxms.nxmc.base.views.View)
    */
   @Override
   public void viewClosed(View view)
   {
      // Do not call dispose from within view state change handler as it may lead to concurrent modofication of view's listener set
      display.asyncExec(() -> dispose());
   }
}
