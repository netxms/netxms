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
package org.netxms.nxmc.base.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.base.VersionInfo;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.xnap.commons.i18n.I18n;

/**
 * "About" dialog
 */
public class AboutDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(AboutDialog.class);

   /**
    * @param parentShell
    */
   public AboutDialog(Shell parentShell)
   {
      super(parentShell);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("About NetXMS Management Client"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      final GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.horizontalSpacing = 0;
      layout.verticalSpacing = 0;
      layout.numColumns = 3;
      dialogArea.setLayout(layout);

      Composite spacer = new Composite(dialogArea, SWT.NONE);
      spacer.setBackground(ThemeEngine.getBackgroundColor("Window.Header"));
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 6;
      spacer.setLayoutData(gd);

      Label logo = new Label(dialogArea, SWT.NONE);
      logo.setBackground(ThemeEngine.getBackgroundColor("Window.Header"));
      final Image image = ResourceManager.getImage("icons/about.png");
      logo.setImage(image);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      gd.grabExcessVerticalSpace = true;
      logo.setLayoutData(gd);
      logo.addDisposeListener((e) -> image.dispose());

      Composite textArea = new Composite(dialogArea, SWT.NONE);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      textArea.setLayoutData(gd);
      GridLayout textAreaLayout = new GridLayout();
      textAreaLayout.marginWidth = 8;
      textAreaLayout.marginHeight = 8;
      textAreaLayout.verticalSpacing = 10;
      textArea.setLayout(textAreaLayout);

      Text versionText = new Text(textArea, SWT.MULTI | SWT.READ_ONLY);
      versionText.setFont(JFaceResources.getBannerFont());
      versionText.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      textArea.setBackground(versionText.getBackground());

      NXCSession session = Registry.getSession();
      StringBuilder sb = new StringBuilder();
      sb.append("NetXMS Management Client Version ");
      sb.append(VersionInfo.version());
      sb.append("\nCopyright \u00a9 2003-2025 Raden Solutions");
      versionText.setText(sb.toString());

      Text detailsText = new Text(textArea, SWT.MULTI | SWT.READ_ONLY);
      detailsText.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      sb = new StringBuilder();
      sb.append("Connected to ");
      sb.append(session.getServerAddress());
      sb.append(" (");
      sb.append(session.getServerName());
      sb.append(") as ");
      sb.append(session.getUserName());
      sb.append("\nServer version: ");
      sb.append(session.getServerVersion());
      sb.append(" (build ");
      sb.append(session.getServerBuild());
      sb.append(")\nServer ID: ");
      sb.append(Long.toHexString(session.getServerId()));
      if (!Registry.IS_WEB_CLIENT)
      {
         sb.append("\nJava version: ");
         sb.append(System.getProperty("java.version"));
         sb.append("\nJVM: ");
         sb.append(System.getProperty("java.vm.name"));
         sb.append(' ');
         sb.append(System.getProperty("java.vm.version"));
         sb.append("\nOperating system: ");
         sb.append(System.getProperty("os.name"));
         sb.append(" (");
         sb.append(System.getProperty("os.version"));
         sb.append(')');
      }
      detailsText.setText(sb.toString());

      Label separator = new Label(dialogArea, SWT.HORIZONTAL | SWT.SEPARATOR);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = layout.numColumns;
      separator.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, IDialogConstants.CANCEL_ID, i18n.tr("Close"), false);
   }
}
