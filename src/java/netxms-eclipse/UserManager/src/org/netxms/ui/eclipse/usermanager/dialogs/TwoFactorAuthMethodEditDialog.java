/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.ui.eclipse.usermanager.dialogs;

import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.users.TwoFactorAuthenticationMethod;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.usermanager.dialogs.helpers.AbstractMethodBindingConfigurator;
import org.netxms.ui.eclipse.usermanager.dialogs.helpers.CustomMethodBindingConfigurator;
import org.netxms.ui.eclipse.usermanager.dialogs.helpers.MessageMethodBindingConfigurator;
import org.netxms.ui.eclipse.usermanager.dialogs.helpers.TOTPMethodBindingConfigurator;

/**
 * Dialog for editing two-factor authentication method binding for user
 */
public class TwoFactorAuthMethodEditDialog extends Dialog
{
   private String name;
   private Map<String, String> configuration;
   private Map<String, TwoFactorAuthenticationMethod> availableMethods = new HashMap<>();
   private Combo methodSelector;
   private Composite configurationArea;
   private AbstractMethodBindingConfigurator methodConfigurator;

   /**
    * Create new dialog.
    *
    * @param parentShell parent shell
    * @param name name of selected method or null
    * @param configuration configuration for method binding
    */
   public TwoFactorAuthMethodEditDialog(Shell parentShell, String name, Map<String, String> configuration)
   {
      super(parentShell);
      this.name = name;
      this.configuration = new HashMap<String, String>(configuration);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((name != null) ? "Edit Two-Factor Authentication Method" : "Add Two-Factor Authentication Method");
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      
      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      methodSelector = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, "Method", new GridData(SWT.FILL, SWT.CENTER, true, false));
      methodSelector.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            int index = methodSelector.getSelectionIndex();
            if (index == -1)
               return;

            String methodName = methodSelector.getItem(index);
            TwoFactorAuthenticationMethod method = availableMethods.get(methodName);
            createConfigurator(method.getDriver());
         }
      });
      readMethodList();
      if (name != null)
         methodSelector.setEnabled(false);

      configurationArea = new Composite(dialogArea, SWT.NONE);
      configurationArea.setLayout(new FillLayout());
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.minimumWidth = 600;
      configurationArea.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * Read list of available methods from server
    */
   private void readMethodList()
   {
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Get list of available 2FA methods", null, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<TwoFactorAuthenticationMethod> methods = session.get2FAMethods();
            Collections.sort(methods, new Comparator<TwoFactorAuthenticationMethod>() {
               @Override
               public int compare(TwoFactorAuthenticationMethod m1, TwoFactorAuthenticationMethod m2)
               {
                  return m1.getName().compareToIgnoreCase(m2.getName());
               }
            });
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  for(TwoFactorAuthenticationMethod m : methods)
                  {
                     availableMethods.put(m.getName(), m);
                     methodSelector.add(m.getName());
                     if (m.getName().equals(name))
                     {
                        methodSelector.select(methodSelector.getItemCount() - 1);
                        createConfigurator(m.getDriver());
                     }
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of available 2FA methods";
         }
      }.start();
   }

   /**
    * (Re)create method configurator.
    *
    * @param driver selected driver
    */
   private void createConfigurator(String driver)
   {
      if (methodConfigurator != null)
         methodConfigurator.dispose();

      if (driver.equals("TOTP"))
      {
         methodConfigurator = new TOTPMethodBindingConfigurator(configurationArea);
      }
      else if (driver.equals("Message"))
      {
         methodConfigurator = new MessageMethodBindingConfigurator(configurationArea);
      }
      else
      {
         methodConfigurator = new CustomMethodBindingConfigurator(configurationArea);
      }
      getShell().pack();
      if (configuration != null)
         methodConfigurator.setConfiguration(configuration);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      int index = methodSelector.getSelectionIndex();
      if (index == -1)
      {
         MessageDialogHelper.openError(getShell(), "Error", "Authentication method not selected!");
         return;
      }

      name = methodSelector.getItem(index);
      configuration = (methodConfigurator != null) ? methodConfigurator.getConfiguration() : new HashMap<String, String>(0);
      super.okPressed();
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return the configuration
    */
   public Map<String, String> getConfiguration()
   {
      return configuration;
   }
}
