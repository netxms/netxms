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
package org.netxms.ui.eclipse.console;

import java.lang.reflect.InvocationTargetException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.window.Window;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Layout;
import org.eclipse.swt.widgets.ProgressBar;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.internal.StartupThreading.StartupRunnable;
import org.netxms.base.VersionInfo;
import org.netxms.ui.eclipse.console.dialogs.DefaultLoginForm;
import org.netxms.ui.eclipse.tools.ColorCache;

/**
 * Progress monitor window
 */
@SuppressWarnings("restriction")
public class ProgressMonitorWindow extends Window
{
   private ColorCache colors;
   private ProgressBar progressBar;
   private Label progressMessage;
   private Object startMutex = new Object();
   private boolean controlsCreated = false;
   private boolean taskIsRunning = false;
   private InvocationTargetException taskException = null;

   /**
    * @param parentShell
    * @param runnable
    */
   public ProgressMonitorWindow(Shell parentShell)
   {
      super(parentShell);
      setBlockOnOpen(true);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setFullScreen(true);
   }

   /**
    * @see org.eclipse.jface.window.Window#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      colors = new ColorCache(parent);
      
      RGB screenBkgnd = BrandingManager.getInstance().getLoginScreenBackgroundColor(DefaultLoginForm.DEFAULT_SCREEN_BACKGROUND_COLOR);
      parent.setBackground(colors.create(screenBkgnd));
      
      Composite fillerTop = new Composite(parent, SWT.NONE);
      fillerTop.setBackground(colors.create(screenBkgnd));
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = false;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.heightHint = 100;
      fillerTop.setLayoutData(gd);

      RGB formBkgnd = BrandingManager.getInstance().getLoginFormBackgroundColor(DefaultLoginForm.DEFAULT_FORM_BACKGROUND_COLOR);

      final Canvas content = new Canvas(parent, SWT.BORDER | SWT.NO_FOCUS);  // SWT.NO_FOCUS is a workaround for Eclipse/RAP bug 321274
      GridLayout layout = new GridLayout();
      layout.numColumns = 1;
      layout.marginWidth = 30;
      layout.marginHeight = 30;
      layout.horizontalSpacing = 10;
      layout.verticalSpacing = 10;
      content.setLayout(layout);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.CENTER;
      gd.verticalAlignment = SWT.CENTER;
      content.setLayoutData(gd);
      content.setBackground(colors.create(formBkgnd));
      content.setData(RWT.CUSTOM_VARIANT, "LoginForm");

      progressBar = new ProgressBar(content, SWT.SMOOTH | SWT.HORIZONTAL);
      gd = new GridData();
      gd.widthHint = 800;
      progressBar.setLayoutData(gd);
      progressBar.setData(RWT.CUSTOM_VARIANT, "login");

      progressMessage = new Label(content, SWT.NONE);
      progressMessage.setBackground(colors.create(formBkgnd));
      gd = new GridData();
      gd.widthHint = 800;
      progressMessage.setLayoutData(gd);
      progressMessage.setData(RWT.CUSTOM_VARIANT, "login");
      
      Composite fillerBottom = new Composite(parent, SWT.NONE);
      fillerBottom.setBackground(colors.create(screenBkgnd));
      fillerBottom.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      
      Label version = new Label(parent, SWT.NONE);
      version.setText(String.format(Messages.get().LoginForm_Version, VersionInfo.version()));
      version.setBackground(parent.getBackground());
      version.setForeground(colors.create(BrandingManager.getInstance().getLoginScreenTextColor(DefaultLoginForm.DEFAULT_SCREEN_TEXT_COLOR)));
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.verticalAlignment = SWT.BOTTOM;
      version.setLayoutData(gd);
      
      synchronized(startMutex)
      {
         controlsCreated = true;
         startMutex.notify();
      }
      
      return content;
   }

   /**
    * @see org.eclipse.jface.window.Window#getLayout()
    */
   @Override
   protected Layout getLayout()
   {
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      return layout;
   }

   /**
    * Open progress monitor window and execute given runnable in background thread
    * 
    * @param runnable
    */
   public void run(IRunnableWithProgress runnable) throws InvocationTargetException
   {
      taskIsRunning = true;

      final Display display = Display.getCurrent();
      final IProgressMonitor monitor = new IProgressMonitor() {
         @Override
         public void worked(final int work)
         {
            asyncExec(display, () -> progressBar.setSelection(progressBar.getSelection() + work));
         }
         
         @Override
         public void subTask(final String name)
         {
            asyncExec(display, () -> progressMessage.setText(name));
         }
         
         @Override
         public void setTaskName(final String name)
         {
            asyncExec(display, () -> progressMessage.setText(name));
         }

         @Override
         public void setCanceled(boolean value)
         {
         }

         @Override
         public boolean isCanceled()
         {
            return false;
         }

         @Override
         public void internalWorked(double work)
         {
         }

         @Override
         public void done()
         {
            Activator.logInfo("Progress monitor: task called done()");
            asyncExec(display, () -> {
               progressBar.setSelection(progressBar.getMaximum());
               progressMessage.setText("");
            });
         }

         @Override
         public void beginTask(final String name, final int totalWork)
         {
            Activator.logInfo("Progress monitor: begin task \"" + name + "\", total work " + totalWork);
            asyncExec(display, () -> {
               progressBar.setMaximum(totalWork);
               progressBar.setSelection(0);
               progressMessage.setText(name);
            });
         }
      };

      Thread worker = new Thread("LoginJob") {
         @Override
         public void run()
         {
            Activator.logInfo("Progress monitor: worker is waiting for window initialization");
            synchronized(startMutex)
            {
               while(!controlsCreated)
               {
                  try
                  {
                     startMutex.wait();
                  }
                  catch(InterruptedException e)
                  {
                  }
               }
            }

            Activator.logInfo("Progress monitor: running provided task");
            try
            {
               runnable.run(monitor);
               Activator.logInfo("Progress monitor: task completed");
               monitor.setTaskName("Initializing workbench...");
            }
            catch(InvocationTargetException e)
            {
               Activator.logError("Progress monitor: exception in task", e);
               taskException = e;
            }
            catch(InterruptedException e)
            {
            }

            asyncExec(display, () -> close());
         }
      };
      worker.start();

      final StartupRunnable timer = new StartupRunnable() {
         @Override
         public void runWithException() throws Throwable
         {
            if (taskIsRunning)
               display.timerExec(50, this);
         }
      };
      display.timerExec(50, timer);

      open();

      taskIsRunning = false;

      if (taskException != null)
         throw taskException;
   }

   /**
    * Asynchronously execute runnable on given display.
    *
    * @param display display object
    * @param r runnable to execute
    */
   private static void asyncExec(Display display, Runnable r)
   {
      display.asyncExec(new StartupRunnable() {
         @Override
         public void runWithException() throws Throwable
         {
            r.run();
         }
      });
   }
}
