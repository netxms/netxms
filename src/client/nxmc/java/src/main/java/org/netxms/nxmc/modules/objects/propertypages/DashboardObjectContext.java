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
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Object Context" property page for dashboard object
 */
public class DashboardObjectContext extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(DashboardObjectContext.class);

   private Dashboard dashboard;
   private Button checkboxShowContextSelector;
   private Button checkboxShowAsObjectView;
   private LabeledSpinner displayPriority;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public DashboardObjectContext(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(DashboardObjectContext.class).tr("Object Context"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "dashboard-object-context";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return object instanceof Dashboard;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 20;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = new Composite(parent, SWT.NONE);
		
      dashboard = (Dashboard)object;

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      if (!dashboard.isTemplateInstance())
      {
         checkboxShowContextSelector = new Button(dialogArea, SWT.CHECK);
         checkboxShowContextSelector.setText(i18n.tr("Show &context selector in dashboard perspective"));
         checkboxShowContextSelector.setSelection((dashboard.getFlags() & Dashboard.SHOW_CONTEXT_SELECTOR) != 0);
         checkboxShowContextSelector.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false));
      }
         
      checkboxShowAsObjectView = new Button(dialogArea, SWT.CHECK);
      checkboxShowAsObjectView.setText(i18n.tr("&Automatically show this dashboard in object context"));
      checkboxShowAsObjectView.setSelection((dashboard.getFlags() & Dashboard.SHOW_AS_OBJECT_VIEW) != 0);
      checkboxShowAsObjectView.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false));

      displayPriority = new LabeledSpinner(dialogArea, SWT.NONE);
      displayPriority.setLabel(i18n.tr("Display priority (1-65535, 0 for automatic)"));
      displayPriority.setRange(0, 65535);
      displayPriority.setSelection(dashboard.getDisplayPriority());
      displayPriority.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false));

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

      int flags = 0;
      if (!dashboard.isTemplateInstance() && checkboxShowContextSelector.getSelection())
         flags |= Dashboard.SHOW_CONTEXT_SELECTOR;
      if (checkboxShowAsObjectView.getSelection())
         flags |= Dashboard.SHOW_AS_OBJECT_VIEW;

		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      md.setObjectFlags(flags, Dashboard.SHOW_CONTEXT_SELECTOR | Dashboard.SHOW_AS_OBJECT_VIEW);
      md.setDisplayPriority(displayPriority.getSelection());

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating dashboard object context configuration"), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> DashboardObjectContext.this.setValid(true));
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot update dashboard object context configuration");
			}
		}.start();

      return true;
	}
}
