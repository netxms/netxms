/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectCategorySelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "General" property page for NetXMS objects
 */
public class General extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(General.class);

   private Text name;
   private Text alias;
   private ObjectCategorySelector categorySelector;
	private String initialName;
   private String initialAlias;
   private int initialCategory;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public General(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(General.class).tr("General"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "general";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 1;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      // Object ID
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
           i18n.tr("Object ID"), Long.toString(object.getObjectId()), WidgetHelper.DEFAULT_LAYOUT_DATA);
      
		// Object class
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, SWT.DEFAULT,
            i18n.tr("Object class"), object.getObjectClassName(), WidgetHelper.DEFAULT_LAYOUT_DATA);
		
		// Object name
      initialName = object.getObjectName();
      name = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, i18n.tr("Name"),
            initialName, WidgetHelper.DEFAULT_LAYOUT_DATA);
      
      // Object alias
      initialAlias = object.getAlias();
      alias = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, i18n.tr("Alias"),
            initialAlias, WidgetHelper.DEFAULT_LAYOUT_DATA);

      // Category selector
      initialCategory = object.getCategoryId();
      categorySelector = new ObjectCategorySelector(dialogArea, SWT.NONE);
      categorySelector.setLabel(i18n.tr("Category"));
      categorySelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      categorySelector.setCategoryId(initialCategory);

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createControl(Composite parent)
   {
      super.createControl(parent);
      getDefaultsButton().setVisible(false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
      final String newName = name.getText();
      final String newAlias = alias.getText();
      final int newCategory = categorySelector.getCategoryId();
      if (newName.equals(initialName) && newAlias.equals(initialAlias) && (newCategory == initialCategory))
         return true; // nothing to change

		if (isApply)
			setValid(false);

      final NXCSession session = Registry.getSession();
		final NXCObjectModificationData data = new NXCObjectModificationData(object.getObjectId());
		data.setName(newName);
      data.setAlias(newAlias);
      data.setCategoryId(newCategory);
      new Job(i18n.tr("Updating object properties"), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(data);
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot modify object");
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
							initialName = newName;
                     initialAlias = newAlias;
                     initialCategory = newCategory;
							General.this.setValid(true);
						}
					});
				}
			}
		}.start();

      return true;
	}
}
