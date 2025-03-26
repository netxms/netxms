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
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Rack;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Rack" property page for rack object
 */
public class RackProperties extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(RackProperties.class);

	private Rack rack;
	private LabeledSpinner rackHeight;
	private Combo numberingScheme;
   private Button checkFrontOnly;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public RackProperties(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(RackProperties.class).tr("Rack"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "rackProperties";
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
      return (object instanceof Rack);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
      rack = (Rack)object;

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);

      rackHeight = new LabeledSpinner(dialogArea, SWT.NONE);
      rackHeight.setLabel(i18n.tr("Height"));
      rackHeight.setRange(1, 50);
      rackHeight.setSelection(rack.getHeight());
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      rackHeight.setLayoutData(gd);
      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      numberingScheme = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY | SWT.DROP_DOWN, i18n.tr("Numbering"), gd);
      numberingScheme.add(i18n.tr("Bottom to top"));
      numberingScheme.add(i18n.tr("Top to bottom"));
      numberingScheme.select(rack.isTopBottomNumbering() ? 1 : 0);
      
      checkFrontOnly = new Button(dialogArea, SWT.CHECK);
      checkFrontOnly.setText(i18n.tr("&Front side only"));
      checkFrontOnly.setSelection(rack.isFrontSideOnly());
      checkFrontOnly.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, false));

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

		final NXCObjectModificationData md = new NXCObjectModificationData(rack.getObjectId());
		md.setHeight(rackHeight.getSelection());
		md.setRackNumberingTopBottom(numberingScheme.getSelectionIndex() == 1);
      md.setObjectFlags(checkFrontOnly.getSelection() ? Rack.FRONT_SIDE_ONLY : 0, Rack.FRONT_SIDE_ONLY);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating properties of rack {0}", rack.getObjectName()), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
			   session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot update rack properties");
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> RackProperties.this.setValid(true));
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
		rackHeight.setSelection(42);
		numberingScheme.select(0);
      checkFrontOnly.setSelection(false);
	}
}
