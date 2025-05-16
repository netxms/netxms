/**
 * NetXMS - open source network management system
 * Copyright (C) 2023 Raden Solutions
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.wizard.ProgressMonitorPart;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;

/**
 * Embedded progress monitor
 */
public class EmbeddedProgressMonitor extends Composite implements IProgressMonitor
{
   ProgressMonitorPart monitor;

   /**
    * Create embedded progress monitor.
    *
    * @param parent parent composite
    * @param style widget style
    * @param showStopButton true to show stop button
    */
   public EmbeddedProgressMonitor(Composite parent, int style, boolean showStopButton)
   {
      super(parent, style);

      FillLayout layout = new FillLayout();
      layout.marginHeight = 5;
      layout.marginWidth = 5;
      setLayout(layout);

      monitor = new ProgressMonitorPart(this, null, showStopButton);
   }

   /**
    * Ensure that given runnable will be executed in UI thread
    *
    * @param r runnable to execute
    */
   private void updateUI(final Runnable r)
   {
      Display display = getDisplay();
      if (Thread.currentThread() == display.getThread())
         r.run(); // run immediately if we're on the UI thread
      else
         display.asyncExec(r);
   }

   /**
    * @see org.eclipse.core.runtime.IProgressMonitor#beginTask(java.lang.String, int)
    */
   @Override
   public void beginTask(String name, int totalWork)
   {
      updateUI(() -> monitor.beginTask(name, totalWork));
   }

   /**
    * @see org.eclipse.core.runtime.IProgressMonitor#done()
    */
   @Override
   public void done()
   {
      updateUI(() -> monitor.done());
   }

   /**
    * @see org.eclipse.core.runtime.IProgressMonitor#internalWorked(double)
    */
   @Override
   public void internalWorked(double work)
   {
      updateUI(() -> monitor.internalWorked(work));
   }

   /**
    * @see org.eclipse.core.runtime.IProgressMonitor#isCanceled()
    */
   @Override
   public boolean isCanceled()
   {
      return monitor.isCanceled();
   }

   /**
    * @see org.eclipse.core.runtime.IProgressMonitor#setCanceled(boolean)
    */
   @Override
   public void setCanceled(boolean value)
   {
      monitor.setCanceled(value);
   }

   /**
    * @see org.eclipse.core.runtime.IProgressMonitor#setTaskName(java.lang.String)
    */
   @Override
   public void setTaskName(String name)
   {
      updateUI(() -> monitor.setTaskName(name));
   }

   /**
    * @see org.eclipse.core.runtime.IProgressMonitor#subTask(java.lang.String)
    */
   @Override
   public void subTask(String name)
   {
      updateUI(() -> monitor.subTask(name));
   }

   /**
    * @see org.eclipse.core.runtime.IProgressMonitor#worked(int)
    */
   @Override
   public void worked(int work)
   {
      updateUI(() -> monitor.worked(work));
   }
}
