/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 RadenSolutions
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
package org.netxms.nxmc.modules.alarms.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.events.AlarmCategory;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.editors.AlarmCategoryEditor;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "General" property page of an Alarm Category
 */
public class General extends PropertyPage
{
   public static final I18n i18n = LocalizationHelper.getI18n(General.class);

   private Text textName;
   private Text textDescription;
   private AlarmCategoryEditor editor;
   private AlarmCategory category;
   private NXCSession session;

   /**
    * Constructor
    * 
    * @param editor editor for alarm category
    */
   public General(AlarmCategoryEditor editor)
   {
      super(LocalizationHelper.getI18n(General.class).tr("General"));
      this.editor = editor;
      category = editor.getObjectAsItem();
   }
   
   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      session = Registry.getSession();
      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);

      // Category ID
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT, i18n.tr("Category ID"),
            Long.toString(category.getId()), WidgetHelper.DEFAULT_LAYOUT_DATA);

      // Category name
      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, i18n.tr("Category name"), category.getName(),
            WidgetHelper.DEFAULT_LAYOUT_DATA);

      // Category description
      textDescription = WidgetHelper.createLabeledText(dialogArea, SWT.MULTI | SWT.BORDER, SWT.DEFAULT, i18n.tr("Description"),
            category.getDescription(), WidgetHelper.DEFAULT_LAYOUT_DATA);
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
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      final String newName = textName.getText();
      final String newDescription = textDescription.getText();

      if (newName.equals(""))
      {
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Category name cannot be empty!"));
         return false;
      }
      
      if (newName.equals(category.getName()) && newDescription.equals(category.getDescription()))
         return true; // Nothing to apply
      
      if (!newName.equals(category.getName()))
      {
         AlarmCategory c = session.findAlarmCategoryByName(newName);
         if (c != null)
         {
            MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Category name already exists!"));
            return false;
         }
         category.setName(newName);
      }
      
      category.setDescription(newDescription);
      
      editor.modify();
      
      return true;
   }
}
