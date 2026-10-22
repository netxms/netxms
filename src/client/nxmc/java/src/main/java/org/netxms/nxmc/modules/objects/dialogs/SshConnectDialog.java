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
package org.netxms.nxmc.modules.objects.dialogs;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.SshKeyPair;
import org.netxms.client.objects.AbstractNode;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.PasswordInputField;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ssh.SshConnectionSettings;
import org.netxms.nxmc.modules.objects.ssh.SshConnectionSettings.Mode;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for selecting SSH terminal transport and credentials. Last used mode, login, key and port are remembered per node
 * in preference store; password is never stored.
 */
public class SshConnectDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(SshConnectDialog.class);

   private final AbstractNode node;
   private final SshConnectionSettings settings;
   private final String prefPrefix;
   private Button radioProxy;
   private Button radioDirect;
   private Button checkStoredCredentials;
   private LabeledText textLogin;
   private PasswordInputField textPassword;
   private LabeledText textPort;
   private Combo comboKey;
   private LabeledText textKeyFile;
   private List<SshKeyPair> keyList = null;

   /**
    * Create dialog.
    *
    * @param parentShell parent shell
    * @param node target node
    */
   public SshConnectDialog(Shell parentShell, AbstractNode node)
   {
      super(parentShell);
      this.node = node;
      this.settings = new SshConnectionSettings(node);
      this.prefPrefix = "SshTerminal." + node.getObjectId() + ".";
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("SSH Terminal - {0}", node.getObjectName()));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      PreferenceStore ps = PreferenceStore.getInstance();
      Mode mode = settings.getMode();
      try
      {
         String savedMode = ps.getString(prefPrefix + "mode");
         if (!savedMode.isEmpty())
            mode = Mode.valueOf(savedMode);
      }
      catch(IllegalArgumentException e)
      {
      }
      boolean storedCredentials = ps.getBoolean(prefPrefix + "storedCredentials");
      String savedLogin = ps.getString(prefPrefix + "login");
      int savedPort = ps.getInt(prefPrefix + "port");
      int savedKeyId = ps.getInt(prefPrefix + "keyId");

      Group modeGroup = new Group(dialogArea, SWT.NONE);
      modeGroup.setText(i18n.tr("Connection"));
      modeGroup.setLayout(new GridLayout());
      modeGroup.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      radioProxy = new Button(modeGroup, SWT.RADIO);
      radioProxy.setText(i18n.tr("Through NetXMS SSH proxy (session established by proxy agent)"));
      radioProxy.setSelection(mode == Mode.PROXY);

      radioDirect = new Button(modeGroup, SWT.RADIO);
      radioDirect.setText(i18n.tr("Direct SSH from console over TCP proxy (end-to-end encrypted)"));
      radioDirect.setSelection(mode == Mode.DIRECT);

      checkStoredCredentials = new Button(dialogArea, SWT.CHECK);
      checkStoredCredentials.setText(i18n.tr("Use credentials stored in node properties"));
      checkStoredCredentials.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      checkStoredCredentials.setSelection((mode == Mode.PROXY) && (storedCredentials || savedLogin.isEmpty()));

      textLogin = new LabeledText(dialogArea, SWT.NONE);
      textLogin.setLabel(i18n.tr("Login"));
      textLogin.setText(savedLogin.isEmpty() ? node.getSshLogin() : savedLogin);
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.widthHint = 250;
      textLogin.setLayoutData(gd);

      textPassword = new PasswordInputField(dialogArea, SWT.NONE);
      textPassword.setLabel(i18n.tr("Password / key passphrase"));
      textPassword.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      comboKey = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Server key"), new GridData(SWT.FILL, SWT.CENTER, true, false));
      comboKey.add(i18n.tr("<none>"));
      comboKey.select(0);

      textPort = new LabeledText(dialogArea, SWT.NONE);
      textPort.setLabel(i18n.tr("Port"));
      textPort.setText(Integer.toString((savedPort > 0) ? savedPort : settings.getPort()));
      textPort.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Private key file is read by SSH client running in the console; in web client that would be a file on web server host
      if (!Registry.IS_WEB_CLIENT)
      {
         textKeyFile = new LabeledText(dialogArea, SWT.NONE);
         textKeyFile.setLabel(i18n.tr("Private key file (direct mode)"));
         textKeyFile.setText(ps.getString(prefPrefix + "keyFile"));
         textKeyFile.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      }

      SelectionAdapter updateListener = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            updateFieldState();
         }
      };
      radioProxy.addSelectionListener(updateListener);
      radioDirect.addSelectionListener(updateListener);
      checkStoredCredentials.addSelectionListener(updateListener);
      updateFieldState();

      loadSshKeyList(savedKeyId);
      return dialogArea;
   }

   /**
    * Enable/disable fields according to selected mode
    */
   private void updateFieldState()
   {
      boolean proxyMode = radioProxy.getSelection();
      checkStoredCredentials.setEnabled(proxyMode);
      boolean ownCredentials = !proxyMode || !checkStoredCredentials.getSelection();
      textLogin.setEditable(ownCredentials);
      textPassword.setEditable(ownCredentials);
      comboKey.setEnabled(proxyMode && ownCredentials);
      if (textKeyFile != null)
         textKeyFile.setEditable(!proxyMode);
      textPort.setEditable(!proxyMode);
   }

   /**
    * Load list of server-side SSH keys into combo box
    *
    * @param selectedKeyId key ID to select
    */
   private void loadSshKeyList(final int selectedKeyId)
   {
      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Loading SSH key list"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<SshKeyPair> list = session.getSshKeys(true);
            runInUIThread(() -> {
               if (comboKey.isDisposed())
                  return;
               keyList = list;
               int selection = 0;
               for(int i = 0; i < keyList.size(); i++)
               {
                  SshKeyPair k = keyList.get(i);
                  comboKey.add(k.getName());
                  if (k.getId() == selectedKeyId)
                     selection = i + 1;
               }
               comboKey.select(selection);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load SSH key list");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      Mode mode = radioDirect.getSelection() ? Mode.DIRECT : Mode.PROXY;
      boolean storedCredentials = (mode == Mode.PROXY) && checkStoredCredentials.getSelection();

      int port = settings.getPort();
      if (mode == Mode.DIRECT)
      {
         try
         {
            port = Integer.parseInt(textPort.getText().trim());
         }
         catch(NumberFormatException e)
         {
            port = 0;
         }
         if ((port < 1) || (port > 65535))
         {
            MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter valid port number"));
            return;
         }
      }

      String login = textLogin.getText().trim();
      if (!storedCredentials && login.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter login name"));
         return;
      }

      int keyId = 0;
      int keySelection = comboKey.getSelectionIndex();
      if ((keyList != null) && (keySelection > 0) && (keySelection <= keyList.size()))
         keyId = keyList.get(keySelection - 1).getId();

      settings.setMode(mode);
      settings.setLogin(storedCredentials ? null : login);
      settings.setPassword(textPassword.getText());
      settings.setKeyId(keyId);
      settings.setKeyFile((textKeyFile != null) ? textKeyFile.getText().trim() : null);
      settings.setPort(port);

      PreferenceStore ps = PreferenceStore.getInstance();
      ps.set(prefPrefix + "mode", mode.name());
      ps.set(prefPrefix + "storedCredentials", storedCredentials);
      ps.set(prefPrefix + "login", login);
      ps.set(prefPrefix + "keyId", keyId);
      if (textKeyFile != null)
         ps.set(prefPrefix + "keyFile", settings.getKeyFile());
      ps.set(prefPrefix + "port", port);

      super.okPressed();
   }

   /**
    * Get selected settings (valid after dialog was closed with OK).
    *
    * @return selected settings
    */
   public SshConnectionSettings getSettings()
   {
      return settings;
   }
}
