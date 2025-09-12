/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DashboardTemplate;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Template" property page for dashboard object
 */
public class DashboardTemplateConfiguration extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(DashboardTemplateConfiguration.class);

   private DashboardTemplate dashboard;
   private LabeledText nameTemplate;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public DashboardTemplateConfiguration(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(DashboardTemplateConfiguration.class).tr("Template"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "dashboard-template";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return object instanceof DashboardTemplate;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 21;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = new Composite(parent, SWT.NONE);
		
      dashboard = (DashboardTemplate)object;

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      nameTemplate = new LabeledText(dialogArea, SWT.NONE);
      nameTemplate.setLabel(i18n.tr("Name template (supports macros)"));
      nameTemplate.setTextLimit(255);
      nameTemplate.setText(dashboard.getNameTemplate());
      nameTemplate.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
		if (isApply)
			setValid(false);

		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      md.setDashboardNameTemplate(nameTemplate.getText());

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating dashboard template configuration"), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> DashboardTemplateConfiguration.this.setValid(true));
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot update dashboard template configuration");
			}
		}.start();

      return true;
	}
}
