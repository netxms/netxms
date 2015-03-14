/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IPartListener;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;

/**
 * Helper class to handle view refresh
 */
public class ViewRefreshController implements IPartListener
{
   private Runnable timer;
   private Runnable handler;
   private VisibilityValidator validator;
   private IWorkbenchPage page;
   private Display display;
   private String id;
   private String secondaryId;
   private int interval;
   private boolean disposed = false;
   
   /**
    * Create refresh controller attached to given view part.
    * 
    * @param part
    * @param interval
    * @param handler
    */
   public ViewRefreshController(final IViewPart part, int interval, Runnable handler)
   {
      this(part, interval, handler, null);
   }
   
   /**
    * Create refresh controller attached to given view part.
    * 
    * @param part
    * @param interval
    * @param handler
    * @param validator
    */
   public ViewRefreshController(final IViewPart part, int interval, Runnable handler, VisibilityValidator validator)
   {
      this.handler = handler;
      this.validator = validator;
      this.interval = interval;
      id = part.getSite().getId();
      secondaryId = ((IViewSite)part.getSite()).getSecondaryId();
      page = part.getSite().getPage();
      display = page.getWorkbenchWindow().getShell().getDisplay();
      timer = new Runnable() {
         @Override
         public void run()
         {
            if (page.isPartVisible(part) &&
                ((ViewRefreshController.this.validator == null) || ViewRefreshController.this.validator.isVisible()))
            {
               ViewRefreshController.this.handler.run();
            }
            display.timerExec((ViewRefreshController.this.interval > 0) ? ViewRefreshController.this.interval * 1000 : -1, this);
         }
      };
      page.addPartListener(this);
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
      
      page.removePartListener(this);
      display.timerExec(-1, timer);
      disposed = true;
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.IPartListener#partActivated(org.eclipse.ui.IWorkbenchPart)
    */
   @Override
   public void partActivated(IWorkbenchPart part)
   {
      if ((interval > 0) &&
          part.getSite().getId().equals(id) && 
          (part.getSite() instanceof IViewSite) &&
          compareStrings(((IViewSite)part.getSite()).getSecondaryId(), secondaryId) &&
          ((ViewRefreshController.this.validator == null) || ViewRefreshController.this.validator.isVisible()))
      {
         handler.run();
      }
   }
   
   /**
    * @param s1
    * @param s2
    * @return
    */
   static boolean compareStrings(String s1, String s2)
   {
      if (s1 == null)
         return (s2 == null);
      return s1.equals(s2);
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.IPartListener#partBroughtToTop(org.eclipse.ui.IWorkbenchPart)
    */
   @Override
   public void partBroughtToTop(IWorkbenchPart part)
   {
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.IPartListener#partClosed(org.eclipse.ui.IWorkbenchPart)
    */
   @Override
   public void partClosed(IWorkbenchPart part)
   {
      if (part.getSite().getId().equals(id) && 
            (part.getSite() instanceof IViewSite) &&
            compareStrings(((IViewSite)part.getSite()).getSecondaryId(), secondaryId))
      {
         dispose();
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.IPartListener#partDeactivated(org.eclipse.ui.IWorkbenchPart)
    */
   @Override
   public void partDeactivated(IWorkbenchPart part)
   {
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.IPartListener#partOpened(org.eclipse.ui.IWorkbenchPart)
    */
   @Override
   public void partOpened(IWorkbenchPart part)
   {
   }
}
