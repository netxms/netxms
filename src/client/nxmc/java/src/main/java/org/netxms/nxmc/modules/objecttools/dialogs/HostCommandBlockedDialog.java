/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.objecttools.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Link;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.ExternalWebBrowser;
import org.netxms.nxmc.tools.SandboxHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Informational dialog shown when a local command object tool cannot run because the client is inside a restricted Flatpak sandbox
 * without host command execution permission. Presents the exact {@code flatpak override} command to unlock the feature (selectable
 * and copyable) and a link to the documentation.
 */
public class HostCommandBlockedDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(HostCommandBlockedDialog.class);

   /**
    * @param parentShell parent shell
    */
   public HostCommandBlockedDialog(Shell parentShell)
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
      newShell.setText(i18n.tr("Host Command Execution Blocked"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.numColumns = 1;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      Composite headerArea = new Composite(dialogArea, SWT.NONE);
      GridLayout headerLayout = new GridLayout();
      headerLayout.numColumns = 2;
      headerLayout.marginWidth = 0;
      headerLayout.marginHeight = 0;
      headerLayout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      headerArea.setLayout(headerLayout);
      headerArea.setLayoutData(new GridData(SWT.CENTER, SWT.TOP, true, false));

      Label icon = new Label(headerArea, SWT.NONE);
      icon.setImage(parent.getDisplay().getSystemImage(SWT.ICON_WARNING));
      icon.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, false));

      Label header = new Label(headerArea, SWT.LEFT);
      header.setText(i18n.tr("This Flatpak build runs in a restricted sandbox"));
      header.setFont(JFaceResources.getBannerFont());
      header.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false));

      Label prose = new Label(dialogArea, SWT.WRAP);
      prose.setText(i18n.tr(
            "Executing commands on the host machine is not permitted in this sandbox, so this local command tool cannot run. " +
            "To enable host command execution, run the command below in a host terminal and restart the application:"));
      GridData gd = new GridData(SWT.FILL, SWT.TOP, true, false);
      gd.widthHint = 480;
      prose.setLayoutData(gd);

      Composite commandArea = new Composite(dialogArea, SWT.NONE);
      GridLayout commandLayout = new GridLayout();
      commandLayout.numColumns = 2;
      commandLayout.marginWidth = 0;
      commandLayout.marginHeight = 0;
      commandLayout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      commandArea.setLayout(commandLayout);
      commandArea.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

      Text command = new Text(commandArea, SWT.READ_ONLY | SWT.BORDER | SWT.SINGLE);
      command.setText(SandboxHelper.getHostAccessOverrideCommand());
      command.setFont(JFaceResources.getTextFont());
      command.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Button copyButton = new Button(commandArea, SWT.PUSH);
      copyButton.setText(i18n.tr("&Copy"));
      copyButton.setLayoutData(new GridData(SWT.RIGHT, SWT.CENTER, false, false));
      copyButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            WidgetHelper.copyToClipboard(SandboxHelper.getHostAccessOverrideCommand());
         }
      });

      Link docLink = new Link(dialogArea, SWT.NONE);
      docLink.setText(i18n.tr("See <a>the documentation</a> for details."));
      GridData docLinkGd = new GridData(SWT.CENTER, SWT.CENTER, true, false);
      docLinkGd.verticalIndent = WidgetHelper.DIALOG_SPACING;
      docLink.setLayoutData(docLinkGd);
      docLink.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            ExternalWebBrowser.open(SandboxHelper.getDocumentationUrl());
         }
      });

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, IDialogConstants.OK_ID, i18n.tr("Close"), true);
   }
}
