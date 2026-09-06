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
import org.netxms.client.objects.Facility;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Facility" property page for facility object
 */
public class FacilityProperties extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(FacilityProperties.class);

   private Facility facility;
   private LabeledSpinner settlementLag;
   private LabeledText providerId;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public FacilityProperties(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(FacilityProperties.class).tr("Facility"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "facilityProperties";
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
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof Facility);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      facility = (Facility)object;

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      settlementLag = new LabeledSpinner(dialogArea, SWT.NONE);
      settlementLag.setLabel(i18n.tr("Settlement lag (days)"));
      settlementLag.setRange(0, 365);
      settlementLag.setSelection(facility.getSettlementLag());
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      settlementLag.setLayoutData(gd);

      providerId = new LabeledText(dialogArea, SWT.NONE);
      providerId.setLabel(i18n.tr("Computation provider (empty to disable KPI engine)"));
      providerId.getTextControl().setTextLimit(63);
      providerId.setText(facility.getProviderId());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      providerId.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      if (isApply)
         setValid(false);

      final NXCObjectModificationData md = new NXCObjectModificationData(facility.getObjectId());
      md.setSettlementLag(settlementLag.getSelection());
      md.setProviderId(providerId.getText().trim());

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating properties of facility {0}", facility.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update facility properties");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> FacilityProperties.this.setValid(true));
         }
      }.start();
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      super.performDefaults();
      settlementLag.setSelection(5);
      providerId.setText("");
   }
}
