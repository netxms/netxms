/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.widgets.UserSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Chat bot user mapping dialog - maps platform peer ID to NetXMS user. When editing an existing
 * mapping the peer ID field is read-only (only the mapped user can be changed).
 */
public class ChatBotUserMappingDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(ChatBotUserMappingDialog.class);

   private String peerId;
   private int userId;
   private boolean editMode;
   private LabeledText textPeerId;
   private UserSelector userSelector;

   /**
    * Create user mapping dialog.
    *
    * @param parentShell parent shell
    * @param peerId peer ID of mapping being edited or null to create new mapping
    * @param userId currently mapped user ID (ignored when creating new mapping)
    */
   public ChatBotUserMappingDialog(Shell parentShell, String peerId, int userId)
   {
      super(parentShell);
      this.peerId = peerId;
      this.userId = userId;
      editMode = (peerId != null);
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
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      textPeerId = new LabeledText(dialogArea, SWT.NONE);
      textPeerId.setLabel(i18n.tr("Platform peer ID (chat ID, JID, etc.)"));
      textPeerId.getTextControl().setTextLimit(127);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      textPeerId.setLayoutData(gd);
      if (editMode)
      {
         textPeerId.setText(peerId);
         textPeerId.setEditable(false);
      }

      userSelector = new UserSelector(dialogArea, SWT.NONE);
      userSelector.setLabel(i18n.tr("User"));
      if (editMode)
         userSelector.setUserId(userId);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      userSelector.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(editMode ? i18n.tr("Edit User Mapping") : i18n.tr("Add User Mapping"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      String id = textPeerId.getText().trim();
      if (id.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Peer ID should not be empty"));
         return;
      }

      if (userSelector.getUserId() < 0)
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("User should be selected"));
         return;
      }

      peerId = id;
      userId = userSelector.getUserId();
      super.okPressed();
   }

   /**
    * Get peer ID.
    *
    * @return peer ID
    */
   public String getPeerId()
   {
      return peerId;
   }

   /**
    * Get mapped user ID.
    *
    * @return mapped user ID
    */
   public int getUserId()
   {
      return userId;
   }
}
