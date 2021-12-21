/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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

import java.util.Date;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.DateTimeSelector;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for entering maintenance schedule
 */
public class MaintanenceScheduleDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(MaintanenceScheduleDialog.class);

   private Date startDate;
   private Date endDate;
   private String comments;
   private DateTimeSelector startDateSelector;
   private DateTimeSelector endDateSelector;
   private Label labelStartDate;
   private Label labelEndDate;
   private LabeledText commentsEditor;

   /**
    * @param parentShell
    */
   public MaintanenceScheduleDialog(Shell parentShell)
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
      newShell.setText(i18n.tr("Schedule Maintenance"));
   }
   
   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      labelStartDate = new Label(dialogArea, SWT.NONE);
      labelStartDate.setText(i18n.tr("Start time"));
            
      startDateSelector = new DateTimeSelector(dialogArea, SWT.NONE);
      startDateSelector.setValue(new Date());
      startDateSelector.setToolTipText(i18n.tr("Start time"));
      
      labelEndDate = new Label(dialogArea, SWT.NONE);
      labelEndDate.setText(i18n.tr("End time"));
      
      endDateSelector = new DateTimeSelector(dialogArea, SWT.NONE);
      endDateSelector.setValue(new Date());
      endDateSelector.setToolTipText(i18n.tr("End time"));
      
      commentsEditor = new LabeledText(dialogArea, SWT.NONE);
      commentsEditor.setLabel(i18n.tr("Comments"));
      commentsEditor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      
      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      startDate = startDateSelector.getValue();
      endDate = endDateSelector.getValue();
      comments = commentsEditor.getText();
      if (startDate.after(endDate))
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Start time must be earlier than end time!"));
         return;
      }
      super.okPressed();
   }

   /**
    * Get start time
    * 
    * @return start time
    */
   public Date getStartTime()
   {
      return startDate;
   }

   /**
    * Get end time
    * 
    * @return end time
    */
   public Date getEndTime()
   {
      return endDate;
   }

   /**
    * Get comments
    * 
    * @return comments
    */
   public String getComments()
   {
      return comments;
   }
}
