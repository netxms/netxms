/**
 * NetXMS - open source network management system
 * Copyright (C) 2022 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Windows performance counter specific configuration
 */
public class WinPerf extends AbstractDCIPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(WinPerf.class);
   
	private DataCollectionItem dci;
   private LabeledSpinner sampleCount;

   
   public WinPerf(DataCollectionObjectEditor editor)
   {
      super(LocalizationHelper.getI18n(WinPerf.class).tr("Windows Performace Counters"), editor);
   }

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
	   Composite pageArea = (Composite)super.createContents(parent);
		dci = editor.getObjectAsItem();
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      pageArea.setLayout(layout);

      sampleCount = new LabeledSpinner(pageArea, SWT.NONE);
      sampleCount.setLabel(i18n.tr("Sample count for average value calculation (0 to disable)"));
      sampleCount.setRange(0, 65535);
      sampleCount.setSelection(dci.getSampleCount());
      
      
		return pageArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	@Override
	protected boolean applyChanges(final boolean isApply)
	{    
      dci.setSampleCount(sampleCount.getSelection());
		editor.modify();
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
      sampleCount.setSelection(0);
	}
}
