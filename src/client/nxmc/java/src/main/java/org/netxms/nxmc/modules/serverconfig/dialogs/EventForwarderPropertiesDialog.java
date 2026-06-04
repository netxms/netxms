/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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

import java.util.Collections;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.EventForwarder;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Event forwarder properties dialog
 */
public class EventForwarderPropertiesDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EventForwarderPropertiesDialog.class);

   private EventForwarder forwarder;
   private LabeledText textName;
   private LabeledText textDescription;
   private LabeledText textConfiguration;
   private Combo comboDriverName;

   /**
    * Create new event forwarder properties dialog.
    *
    * @param parentShell parent shell
    * @param forwarder event forwarder to edit, or null to create a new one
    */
   public EventForwarderPropertiesDialog(Shell parentShell, EventForwarder forwarder)
   {
      super(parentShell);
      this.forwarder = forwarder;
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

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      comboDriverName = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Driver"), gd);

      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(i18n.tr("Name"));
      textName.getTextControl().setTextLimit(63);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);
      textName.getTextControl().addModifyListener(listener -> {
         Control button = getButton(IDialogConstants.OK_ID);
         if (button != null)
            button.setEnabled(!textName.getText().trim().isBlank());
      });

      textDescription = new LabeledText(dialogArea, SWT.NONE);
      textDescription.setLabel(i18n.tr("Description"));
      textDescription.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textDescription.setLayoutData(gd);

      textConfiguration = new LabeledText(dialogArea, SWT.NONE, SWT.MULTI | SWT.BORDER);
      textConfiguration.setLabel(i18n.tr("Driver configuration"));
      textConfiguration.getTextControl().setFont(JFaceResources.getTextFont());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 300;
      gd.widthHint = 700;
      textConfiguration.setLayoutData(gd);

      if (forwarder != null)
      {
         textName.setText(forwarder.getName());
         textDescription.setText(forwarder.getDescription());
         textConfiguration.setText(forwarder.getConfiguration());
      }

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Get event forwarder driver names"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<String> drivers = session.getEventForwarderDrivers();
            Collections.sort(drivers, String.CASE_INSENSITIVE_ORDER);
            runInUIThread(() -> updateUI(drivers));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get event forwarder driver names");
         }
      }.start();

      return dialogArea;
   }

   /**
    * Populate driver combo
    *
    * @param drivers list of available drivers
    */
   private void updateUI(List<String> drivers)
   {
      int index = 0;
      if (!drivers.isEmpty())
      {
         for(int i = 0; i < drivers.size(); i++)
         {
            String item = drivers.get(i);
            comboDriverName.add(item);
            if ((forwarder != null) && item.equals(forwarder.getDriverName()))
               index = i;
         }
         comboDriverName.select(index);
      }
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((forwarder != null) ? i18n.tr("Update event forwarder") : i18n.tr("Create event forwarder"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (textName.getText().trim().isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Event forwarder name should not be empty"));
         return;
      }

      if (comboDriverName.getSelectionIndex() == -1)
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Event forwarder driver should be selected"));
         return;
      }

      if (forwarder == null)
         forwarder = new EventForwarder();

      forwarder.setName(textName.getText().trim());
      forwarder.setDescription(textDescription.getText());
      forwarder.setDriverName(comboDriverName.getItem(comboDriverName.getSelectionIndex()));
      forwarder.setConfiguration(textConfiguration.getText());
      super.okPressed();
   }

   /**
    * Return updated event forwarder
    *
    * @return updated event forwarder
    */
   public EventForwarder getEventForwarder()
   {
      return forwarder;
   }
}
