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
package org.netxms.nxmc.base.dialogs;

import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Link;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.VersionInfo;
import org.netxms.client.ProtocolVersionException;
import org.netxms.nxmc.BrandingManager;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.ExternalWebBrowser;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog shown when server and client use incompatible versions of communication protocol. Presents both product versions and, unless
 * disabled by branding, links to the release directory containing a client matching the server.
 */
public class ProtocolVersionMismatchDialog extends Dialog
{
   private static final Pattern VERSION_PATTERN = Pattern.compile("^(\\d+)\\.(\\d+)");

   private final I18n i18n = LocalizationHelper.getI18n(ProtocolVersionMismatchDialog.class);

   private String serverVersion;
   private String serverBuild;

   /**
    * @param parentShell parent shell
    * @param exception exception thrown by client library
    */
   public ProtocolVersionMismatchDialog(Shell parentShell, ProtocolVersionException exception)
   {
      super(parentShell);
      serverVersion = exception.getServerVersion();
      serverBuild = exception.getServerBuild();
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Incompatible Client Version"));
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
      icon.setImage(parent.getDisplay().getSystemImage(SWT.ICON_ERROR));
      icon.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, false));

      Label header = new Label(headerArea, SWT.LEFT);
      header.setText(i18n.tr("This management client cannot connect to this server"));
      header.setFont(JFaceResources.getBannerFont());
      header.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false));

      Composite versionArea = new Composite(dialogArea, SWT.NONE);
      GridLayout versionLayout = new GridLayout();
      versionLayout.numColumns = 2;
      versionLayout.marginWidth = 0;
      versionLayout.marginHeight = 0;
      versionLayout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      versionArea.setLayout(versionLayout);
      versionArea.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, false));

      createVersionLabels(versionArea, i18n.tr("Server:"), formatServerVersion());
      createVersionLabels(versionArea, i18n.tr("Client:"), VersionInfo.version());

      String releaseBranch = getReleaseBranch(serverVersion);
      if ((releaseBranch != null) && BrandingManager.isClientDownloadLinkEnabled())
      {
         final String releaseUrl = getReleaseUrl(releaseBranch);
         Link link = new Link(dialogArea, SWT.NONE);
         link.setText(Registry.IS_WEB_CLIENT ?
               i18n.tr("Server and client must have the same major and minor version. The deployed web client does not match the server. Deploy a matching web client from <a>netxms.org/download/releases/{0}</a>.", releaseBranch) :
               i18n.tr("Server and client must have the same major and minor version. Download a matching client from <a>netxms.org/download/releases/{0}</a>.", releaseBranch));
         GridData gd = new GridData(SWT.FILL, SWT.TOP, true, false);
         gd.widthHint = 480;
         link.setLayoutData(gd);
         link.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               ExternalWebBrowser.open(releaseUrl);
            }
         });
      }
      else
      {
         Label prose = new Label(dialogArea, SWT.WRAP);
         prose.setText(Registry.IS_WEB_CLIENT ?
               i18n.tr("Server and client must have the same major and minor version. The deployed web client does not match the server.") :
               i18n.tr("Server and client must have the same major and minor version."));
         GridData gd = new GridData(SWT.FILL, SWT.TOP, true, false);
         gd.widthHint = 480;
         prose.setLayoutData(gd);
      }

      return dialogArea;
   }

   /**
    * Create pair of labels describing single version.
    *
    * @param parent parent composite
    * @param caption version caption
    * @param version version text
    */
   private static void createVersionLabels(Composite parent, String caption, String version)
   {
      Label captionLabel = new Label(parent, SWT.LEFT);
      captionLabel.setText(caption);
      captionLabel.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false));

      Label versionLabel = new Label(parent, SWT.LEFT);
      versionLabel.setText(version);
      versionLabel.setFont(JFaceResources.getBannerFont());
      versionLabel.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false));
   }

   /**
    * Format server version, appending build tag if server provided one.
    *
    * @return formatted server version
    */
   private String formatServerVersion()
   {
      if ((serverBuild == null) || serverBuild.isEmpty())
         return serverVersion;
      return serverVersion + " (build " + serverBuild + ")";
   }

   /**
    * Get release branch (major.minor) of given version.
    *
    * @param version product version
    * @return release branch or null if version cannot be parsed
    */
   private static String getReleaseBranch(String version)
   {
      if (version == null)
         return null;
      Matcher matcher = VERSION_PATTERN.matcher(version);
      return matcher.find() ? matcher.group(1) + "." + matcher.group(2) : null;
   }

   /**
    * Get URL of release directory for given release branch.
    *
    * @param releaseBranch release branch in form major.minor
    * @return release directory URL
    */
   private static String getReleaseUrl(String releaseBranch)
   {
      return "https://netxms.org/download/releases/" + releaseBranch + "/";
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
