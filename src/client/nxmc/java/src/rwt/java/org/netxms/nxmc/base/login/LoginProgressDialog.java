/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.base.login;

import java.lang.reflect.InvocationTargetException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.ProgressIndicator;
import org.eclipse.jface.operation.IRunnableContext;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.operation.ModalContext;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.VersionInfo;
import org.netxms.nxmc.AppPropertiesLoader;
import org.netxms.nxmc.BrandingManager;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Full-screen dialog for displaying login progress
 */
public class LoginProgressDialog extends Dialog implements IRunnableContext
{
   private final I18n i18n = LocalizationHelper.getI18n(LoginProgressDialog.class);

   private ProgressIndicator progressIndicator;
   private Label messageArea;
   private ProgressMonitor progressMonitor;
   private String taskName;
   private boolean hideVersion;

   /**
    * @param parentShell
    */
   public LoginProgressDialog(AppPropertiesLoader appProperties)
   {
      super((Shell)null);
      hideVersion = appProperties.getPropertyAsBoolean("loginFormNoVersion", false);
      progressMonitor = new ProgressMonitor();
      setBlockOnOpen(false);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell shell)
   {
      super.configureShell(shell);
      shell.setText(String.format(i18n.tr("%s - Connecting"), BrandingManager.getProductName()));
      shell.setMaximized(true);
      shell.setFullScreen(true);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite outerArea = new Composite(parent, SWT.NONE);
      outerArea.setLayout(new GridLayout());
      outerArea.setLayoutData(new GridData(GridData.FILL_BOTH));

      initializeDialogUnits(outerArea);
      Composite header = new Composite(outerArea, SWT.NONE);
      GridLayout headerLayout = new GridLayout();
      header.setLayout(headerLayout);
      header.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

      Composite innerArea = new Composite(outerArea, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      innerArea.setLayout(layout);
      innerArea.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, true));
      innerArea.setData(RWT.CUSTOM_VARIANT, "LoginForm");

      dialogArea = createDialogArea(innerArea);
      dialogArea.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, false));

      buttonBar = new Composite(innerArea, SWT.NONE);
      GridData gd = new GridData();
      gd.heightHint = 100;
      buttonBar.setLayoutData(gd);

      Composite footer = new Composite(outerArea, SWT.NONE);
      GridLayout footerLayout = new GridLayout();
      footerLayout.numColumns = 2;
      footer.setLayout(footerLayout);
      footer.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      Label copyright = new Label(footer, SWT.LEFT);
      copyright.setText("Copyright \u00a9 2013-2025 Raden Solutions");
      copyright.setLayoutData(new GridData(SWT.LEFT, SWT.BOTTOM, true, false));

      Label version = new Label(footer, SWT.RIGHT);
      version.setText(hideVersion ? "" : VersionInfo.version());
      version.setLayoutData(new GridData(SWT.RIGHT, SWT.BOTTOM, true, false));

      applyDialogFont(outerArea);
      return outerArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.BORDER);
      applyDialogFont(dialogArea);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 16;
      layout.marginWidth = 16;
      layout.verticalSpacing = 8;
      dialogArea.setLayout(layout);

      progressIndicator = new ProgressIndicator(dialogArea);
      GridData gd = new GridData();
      gd.heightHint = convertVerticalDLUsToPixels(9);
      gd.widthHint = parent.getDisplay().getBounds().width / 3;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      progressIndicator.setLayoutData(gd);

      messageArea = new Label(dialogArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      messageArea.setLayoutData(gd);
      messageArea.setText(i18n.tr("Working..."));

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.operation.IRunnableContext#run(boolean, boolean, org.eclipse.jface.operation.IRunnableWithProgress)
    */
   @Override
   public void run(boolean fork, boolean cancelable, IRunnableWithProgress runnable) throws InvocationTargetException, InterruptedException
   {
      open();
      try
      {
         ModalContext.run(runnable, fork, progressMonitor, getShell().getDisplay());
      }
      finally
      {
         close();
      }
   }

   /**
    * Internal progress monitor implementation.
    */
   private class ProgressMonitor implements IProgressMonitor
   {
      /**
       * @see org.eclipse.core.runtime.IProgressMonitor#beginTask(java.lang.String, int)
       */
      @Override
      public void beginTask(String name, int totalWork)
      {
         if (progressIndicator.isDisposed())
            return;

         if ((name != null) && !name.isBlank())
         {
            taskName = name;
            messageArea.setText(name);
         }

         if (totalWork == UNKNOWN)
            progressIndicator.beginAnimatedTask();
         else
            progressIndicator.beginTask(totalWork);
      }

      /**
       * @see org.eclipse.core.runtime.IProgressMonitor#done()
       */
      @Override
      public void done()
      {
         if (!progressIndicator.isDisposed())
         {
            progressIndicator.sendRemainingWork();
            progressIndicator.done();
         }
      }

      /**
       * @see org.eclipse.core.runtime.IProgressMonitor#setTaskName(java.lang.String)
       */
      @Override
      public void setTaskName(String name)
      {
         if ((name != null) && !name.isBlank())
         {
            taskName = name;
         }
         else
         {
            taskName = i18n.tr("Working...");
         }
         messageArea.setText(taskName);
      }

      /**
       * @see org.eclipse.core.runtime.IProgressMonitor#isCanceled()
       */
      @Override
      public boolean isCanceled()
      {
         return false;
      }

      /**
       * @see org.eclipse.core.runtime.IProgressMonitor#setCanceled(boolean)
       */
      @Override
      public void setCanceled(boolean b)
      {
      }

      /**
       * @see org.eclipse.core.runtime.IProgressMonitor#subTask(java.lang.String)
       */
      @Override
      public void subTask(String name)
      {
         if (messageArea.isDisposed())
            return;

         if (name == null)
            messageArea.setText(taskName);
         else
            messageArea.setText(taskName + " : " + name);
      }

      /**
       * @see org.eclipse.core.runtime.IProgressMonitor#worked(int)
       */
      @Override
      public void worked(int work)
      {
         internalWorked(work);
      }

      /**
       * @see org.eclipse.core.runtime.IProgressMonitor#internalWorked(double)
       */
      @Override
      public void internalWorked(double work)
      {
         if (!progressIndicator.isDisposed())
            progressIndicator.worked(work);
      }

      /**
       * @see org.eclipse.core.runtime.IProgressMonitor#clearBlocked()
       */
      @Override
      public void clearBlocked()
      {
      }

      /**
       * @see org.eclipse.core.runtime.IProgressMonitor#setBlocked(org.eclipse.core.runtime.IStatus)
       */
      @Override
      public void setBlocked(IStatus reason)
      {
      }
   }
}
