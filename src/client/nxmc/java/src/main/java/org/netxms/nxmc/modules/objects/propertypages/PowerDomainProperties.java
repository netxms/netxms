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
import org.netxms.client.constants.PowerDomainType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.PowerDomain;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.helpers.PowerDomainTypeLabels;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Power Domain" property page for power domain object
 */
public class PowerDomainProperties extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(PowerDomainProperties.class);

   private PowerDomain domain;
   private LabeledCombo domainType;
   private LabeledText feedTag;
   private LabeledSpinner ratedPower;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public PowerDomainProperties(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(PowerDomainProperties.class).tr("Power Domain"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "powerDomainProperties";
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
      return (object instanceof PowerDomain);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      domain = (PowerDomain)object;

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      domainType = new LabeledCombo(dialogArea, SWT.NONE);
      domainType.setLabel(i18n.tr("Domain type"));
      domainType.setContent(PowerDomainTypeLabels.getAll());
      domainType.select(domain.getDomainType().getValue());
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      domainType.setLayoutData(gd);

      feedTag = new LabeledText(dialogArea, SWT.NONE);
      feedTag.setLabel(i18n.tr("Feed tag"));
      feedTag.getTextControl().setTextLimit(15);
      feedTag.setText(domain.getFeedTag());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      feedTag.setLayoutData(gd);

      ratedPower = new LabeledSpinner(dialogArea, SWT.NONE);
      ratedPower.setLabel(i18n.tr("Rated power (W, 0 = undeclared)"));
      ratedPower.setRange(0, Integer.MAX_VALUE);
      ratedPower.setSelection(domain.getRatedPower());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      ratedPower.setLayoutData(gd);

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

      final NXCObjectModificationData md = new NXCObjectModificationData(domain.getObjectId());
      md.setDomainType(PowerDomainType.getByValue(domainType.getSelectionIndex()));
      md.setFeedTag(feedTag.getText().trim());
      md.setRatedPower(ratedPower.getSelection());

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating properties of power domain {0}", domain.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update power domain properties");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> PowerDomainProperties.this.setValid(true));
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
      domainType.select(PowerDomainType.OTHER.getValue());
      feedTag.setText("");
      ratedPower.setSelection(0);
   }
}
