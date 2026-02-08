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
package org.netxms.nxmc.modules.incidents.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for creating a new incident
 */
public class CreateIncidentDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(CreateIncidentDialog.class);
   private static final String CONFIG_PREFIX = "CreateIncidentDialog";

   private ObjectSelector objectSelector;
   private LabeledText titleField;
   private LabeledText initialCommentField;

   private long objectId;
   private String title;
   private String initialComment;

   /**
    * Create dialog
    *
    * @param parentShell parent shell
    */
   public CreateIncidentDialog(Shell parentShell)
   {
      super(parentShell);
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
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Create Incident"));

      PreferenceStore settings = PreferenceStore.getInstance();
      newShell.setSize(settings.getAsInteger(CONFIG_PREFIX + ".cx", 500), settings.getAsInteger(CONFIG_PREFIX + ".cy", 400));
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
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      objectSelector.setLabel(i18n.tr("Source Object"));
      objectSelector.setObjectClass(AbstractObject.class);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      objectSelector.setLayoutData(gd);

      titleField = new LabeledText(dialogArea, SWT.NONE);
      titleField.setLabel(i18n.tr("Title"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      titleField.setLayoutData(gd);

      initialCommentField = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      initialCommentField.setLabel(i18n.tr("Initial comment"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      initialCommentField.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      objectId = objectSelector.getObjectId();
      if (objectId == 0)
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please select source object"));
         return;
      }

      title = titleField.getText().trim();
      if (title.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter incident title"));
         return;
      }

      initialComment = initialCommentField.getText().trim();
      if (initialComment.isEmpty())
         initialComment = null;

      saveSettings();
      super.okPressed();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
   @Override
   protected void cancelPressed()
   {
      saveSettings();
      super.cancelPressed();
   }

   /**
    * Save dialog settings
    */
   private void saveSettings()
   {
      Point size = getShell().getSize();
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set(CONFIG_PREFIX + ".cx", size.x);
      settings.set(CONFIG_PREFIX + ".cy", size.y);
   }

   /**
    * Get selected object ID
    *
    * @return object ID
    */
   public long getObjectId()
   {
      return objectId;
   }

   /**
    * Get incident title
    *
    * @return title
    */
   public String getTitle()
   {
      return title;
   }

   /**
    * Get initial comment for incident
    *
    * @return initial comment or null
    */
   public String getInitialComment()
   {
      return initialComment;
   }
}
