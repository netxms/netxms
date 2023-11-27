/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.objects.GenericObject;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * @author Victor
 *
 */
public class OtherOptionsTable extends AbstractDCIPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(OtherOptionsTable.class);
   
	private DataCollectionTable dci;
	private Combo agentCacheMode;
   private ObjectSelector relatedObject;
   
   /**
    * Constructor
    * 
    * @param editor
    */
   public OtherOptionsTable(DataCollectionObjectEditor editor)
   {
      super(LocalizationHelper.getI18n(OtherOptionsTable.class).tr("Other Options"), editor);
   }

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
	   Composite dialogArea = (Composite)super.createContents(parent);
		dci = editor.getObjectAsTable();
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      agentCacheMode = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Agent cache mode"), new GridData());
      agentCacheMode.add(i18n.tr("Default"));
      agentCacheMode.add(i18n.tr("On"));
      agentCacheMode.add(i18n.tr("Off"));
      agentCacheMode.select(dci.getCacheMode().getValue());
      
      relatedObject = new ObjectSelector(dialogArea, SWT.NONE, true);
      relatedObject.setLabel("Related object");
      relatedObject.setObjectClass(GenericObject.class);
      relatedObject.setObjectId(dci.getRelatedObject());
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      relatedObject.setLayoutData(gd);
      
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
      dci.setCacheMode(AgentCacheMode.getByValue(agentCacheMode.getSelectionIndex()));
      dci.setRelatedObject(relatedObject.getObjectId());
		editor.modify();
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		agentCacheMode.select(0);
		relatedObject.setObjectId(0);
	}
}
