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
package org.netxms.nxmc.modules.ai.dialogs;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.ai.AiMemoryEntry;
import org.netxms.client.constants.AiMemoryScope;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.modules.users.widgets.UserSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * AI memory entry edit dialog. Scope and scope target can be chosen only for new entries. The entry passed for editing is not
 * changed; the edited copy is available via getEntry().
 */
public class AiMemoryEntryEditDialog extends Dialog
{
   private static final int MAX_TITLE_BYTES = 127;

   private final I18n i18n = LocalizationHelper.getI18n(AiMemoryEntryEditDialog.class);

   private AiMemoryEntry entry;
   private List<AiMemoryScope> scopes = new ArrayList<>(3);
   private LabeledCombo comboScope;
   private UserSelector userSelector;
   private ObjectSelector objectSelector;
   private LabeledText textTitle;
   private LabeledText textContent;
   private Button checkLocked;

   /**
    * Create entry edit dialog.
    *
    * @param parentShell parent shell
    * @param entry entry to edit or null to create new entry
    */
   public AiMemoryEntryEditDialog(Shell parentShell, AiMemoryEntry entry)
   {
      super(parentShell);
      this.entry = entry;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((entry == null) ? i18n.tr("Create Memory Entry") : i18n.tr("Edit Memory Entry"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#isResizable()
    */
   @Override
   protected boolean isResizable()
   {
      return true;
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
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      // Environment scope is offered for new entries only to users who can write it
      boolean canManageMemory = (Registry.getSession().getUserSystemRights() & UserAccessRights.SYSTEM_ACCESS_MANAGE_AI_MEMORY) != 0;
      if ((entry != null) || canManageMemory)
         scopes.add(AiMemoryScope.ENVIRONMENT);
      scopes.add(AiMemoryScope.USER);
      scopes.add(AiMemoryScope.OBJECT);

      comboScope = new LabeledCombo(dialogArea, SWT.NONE);
      comboScope.setLabel(i18n.tr("Scope"));
      for(AiMemoryScope scope : scopes)
         comboScope.add(getScopeName(scope));
      comboScope.select((entry != null) ? Math.max(scopes.indexOf(entry.getScope()), 0) : 0);
      comboScope.setEnabled(entry == null);
      comboScope.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      comboScope.getComboControl().addListener(SWT.Selection, (e) -> updateTargetSelectors());

      userSelector = new UserSelector(dialogArea, SWT.NONE);
      userSelector.setLabel(i18n.tr("User"));
      userSelector.setUserId((entry != null) && (entry.getScope() == AiMemoryScope.USER) ? entry.getScopeId() : Registry.getSession().getUserId());
      userSelector.setEnabled(entry == null);
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.widthHint = 400;
      userSelector.setLayoutData(gd);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, false);
      objectSelector.setLabel(i18n.tr("Object"));
      objectSelector.setObjectClass(AbstractObject.class);
      objectSelector.setObjectId((entry != null) && (entry.getScope() == AiMemoryScope.OBJECT) ? entry.getScopeId() : 0);
      objectSelector.setEnabled(entry == null);
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      objectSelector.setLayoutData(gd);

      textTitle = new LabeledText(dialogArea, SWT.NONE);
      textTitle.setLabel(i18n.tr("Title (unique within scope)"));
      textTitle.getTextControl().setTextLimit(MAX_TITLE_BYTES);
      textTitle.setText((entry != null) ? entry.getTitle() : "");
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      gd.widthHint = 600;
      textTitle.setLayoutData(gd);

      textContent = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.WRAP | SWT.V_SCROLL);
      textContent.setLabel(i18n.tr("Content"));
      textContent.setText((entry != null) ? entry.getContent() : "");
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.horizontalSpan = 2;
      gd.heightHint = 200;
      textContent.setLayoutData(gd);

      checkLocked = new Button(dialogArea, SWT.CHECK);
      checkLocked.setText(i18n.tr("&Locked (model cannot modify or delete)"));
      checkLocked.setSelection((entry != null) && entry.isLocked());
      gd = new GridData(SWT.LEFT, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      checkLocked.setLayoutData(gd);

      updateTargetSelectors();
      if (entry == null)
         textTitle.setFocus();
      else
         textContent.setFocus();

      return dialogArea;
   }

   /**
    * Get display name for scope.
    *
    * @param scope scope
    * @return display name
    */
   private String getScopeName(AiMemoryScope scope)
   {
      switch(scope)
      {
         case ENVIRONMENT:
            return i18n.tr("Environment");
         case USER:
            return i18n.tr("User");
         case OBJECT:
            return i18n.tr("Object");
         default:
            return i18n.tr("Unknown");
      }
   }

   /**
    * Get scope currently selected in combo box.
    *
    * @return selected scope
    */
   private AiMemoryScope getSelectedScope()
   {
      int index = comboScope.getSelectionIndex();
      return ((index >= 0) && (index < scopes.size())) ? scopes.get(index) : AiMemoryScope.UNKNOWN;
   }

   /**
    * Enable the target selector matching the selected scope (only for new entries).
    */
   private void updateTargetSelectors()
   {
      if (entry != null)
         return;
      AiMemoryScope scope = getSelectedScope();
      userSelector.setEnabled(scope == AiMemoryScope.USER);
      objectSelector.setEnabled(scope == AiMemoryScope.OBJECT);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      String title = textTitle.getText().trim();
      if (title.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Title cannot be empty"));
         return;
      }
      if (title.getBytes(StandardCharsets.UTF_8).length > MAX_TITLE_BYTES)
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Title is too long (maximum {0} bytes in UTF-8 encoding)", Integer.toString(MAX_TITLE_BYTES)));
         return;
      }

      String content = textContent.getText().trim();
      if (content.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Content cannot be empty"));
         return;
      }

      if (entry == null)
      {
         AiMemoryScope scope = getSelectedScope();
         int scopeId = 0;
         if (scope == AiMemoryScope.USER)
         {
            scopeId = userSelector.getUserId();
            if (scopeId == 0)
            {
               MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("User must be selected"));
               return;
            }
         }
         else if (scope == AiMemoryScope.OBJECT)
         {
            scopeId = (int)objectSelector.getObjectId();
            if (scopeId == 0)
            {
               MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Object must be selected"));
               return;
            }
         }
         entry = new AiMemoryEntry(scope, scopeId, title, content);
      }
      else
      {
         entry = new AiMemoryEntry(entry);
         entry.setTitle(title);
         entry.setContent(content);
      }
      entry.setLocked(checkLocked.getSelection());
      super.okPressed();
   }

   /**
    * Get edited entry.
    *
    * @return edited or newly created entry
    */
   public AiMemoryEntry getEntry()
   {
      return entry;
   }
}
