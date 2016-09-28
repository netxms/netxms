/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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
package org.netxms.ui.eclipse.alarmviewer.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCSession;
import org.netxms.client.events.AlarmCategory;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "General" property page of an Alarm Category
 */
public class General extends PropertyPage
{
   private Text textName;
   private Text textDescription;
   private String categoryName;
   private String categoryDescription;
   private AlarmCategory object;
   private NXCSession session;
   private boolean applyPressed = false;

   public General()
   {
      super();
      session = ConsoleSharedData.getSession();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      object = (AlarmCategory)getElement().getAdapter(AlarmCategory.class);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);

      // Category ID
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, "Category ID",
            Long.toString(object.getId()), WidgetHelper.DEFAULT_LAYOUT_DATA);

      // Category name
      categoryName = object.getName();
      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, "Category name", categoryName,
            WidgetHelper.DEFAULT_LAYOUT_DATA);

      // Category description
      categoryDescription = object.getDescription();
      textDescription = WidgetHelper.createLabeledText(dialogArea, SWT.MULTI | SWT.BORDER, SWT.DEFAULT, "Description",
            categoryDescription, WidgetHelper.DEFAULT_LAYOUT_DATA);
      textDescription.setTextLimit(255);
      GridData gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.heightHint = 100;
      gd.verticalAlignment = SWT.FILL;
      textDescription.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   protected void applyChanges(final boolean isApply)
   {
      final String newName = textName.getText();
      final String newDescription = textDescription.getText();

      if (newName.equals(categoryName) && newDescription.equals(categoryDescription) && applyPressed)
         return; // Nothing to apply

      if (isApply)
         setValid(false);

      new ConsoleJob("Update alarm category database", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            object.setName(newName);
            object.setDescription(newDescription);

            session.modifyAlarmCategory(object, AlarmCategory.MODIFY_ALARM_CATEGORY);
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     General.this.setValid(true);
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot update alarm category database entry";
         }

      }.start();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      applyChanges(false);
      return true;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected void performApply()
   {
      applyChanges(true);
      applyPressed = true;
   }
}
