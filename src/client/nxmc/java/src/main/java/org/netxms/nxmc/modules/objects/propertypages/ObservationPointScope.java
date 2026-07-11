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
package org.netxms.nxmc.modules.objects.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.ObservationPoint;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Scope" property page for observation point objects
 */
public class ObservationPointScope extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(ObservationPointScope.class);

   private ObservationPoint point;
   private Button checkboxInScope;
   private LabeledText zoneUIN;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public ObservationPointScope(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(ObservationPointScope.class).tr("Scope"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "observationPointScope";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof ObservationPoint);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);
      point = (ObservationPoint)object;

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      checkboxInScope = new Button(dialogArea, SWT.CHECK);
      checkboxInScope.setText(i18n.tr("Point is &in scope (run host matching and collect host data)"));
      checkboxInScope.setSelection(point.isInScope());
      checkboxInScope.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Label hint = new Label(dialogArea, SWT.WRAP);
      hint.setText(i18n.tr("In-scope observation points participate in host-to-node matching and per-host data collection. Leave disabled for replay or test interfaces."));
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.widthHint = 400;
      hint.setLayoutData(gd);

      zoneUIN = new LabeledText(dialogArea, SWT.NONE);
      zoneUIN.setLabel(i18n.tr("Zone UIN for host matching (-1 = inherit from observer)"));
      zoneUIN.setText(Integer.toString(point.getZoneUIN()));
      zoneUIN.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(point.getObjectId());
      md.setInScope(checkboxInScope.getSelection());

      try
      {
         md.setZoneUIN(Integer.parseInt(zoneUIN.getText().trim()));
      }
      catch(NumberFormatException e)
      {
         md.setZoneUIN(-1);
      }

      if (isApply)
         setValid(false);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating observation point scope"), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update observation point scope");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> ObservationPointScope.this.setValid(true));
         }
      }.start();
      return true;
   }
}
