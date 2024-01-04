/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.propertypages.helpers.AbstractDCIPropertyPage;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledCombo;

/**
 * DCI property page "Other Options"
 */
public class OtherOptions extends AbstractDCIPropertyPage
{
   private static final String[] TAGS = { "iface-inbound-bits", "iface-inbound-bytes", "iface-inbound-util", "iface-outbound-bits", "iface-outbound-bytes", "iface-outbound-util", "iface-speed" };

	private DataCollectionItem dci;
	private Button checkShowOnTooltip;
	private Button checkShowInOverview;
   private Button checkCalculateStatus;
   private Button checkHideOnLastValues;
   private LabeledCombo multiplierDegree;
   private LabeledCombo agentCacheMode;
   private LabeledCombo interpretation;
   private ObjectSelector relatedObject;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
	   Composite dialogArea = (Composite)super.createContents(parent);
		dci = editor.getObjectAsItem();

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      checkShowOnTooltip = new Button(dialogArea, SWT.CHECK);
      checkShowOnTooltip.setText(Messages.get().NetworkMaps_ShowInTooltips);
      checkShowOnTooltip.setSelection(dci.isShowOnObjectTooltip());

      checkShowInOverview = new Button(dialogArea, SWT.CHECK);
      checkShowInOverview.setText(Messages.get().OtherOptions_ShowlastValueInObjectOverview);
      checkShowInOverview.setSelection(dci.isShowInObjectOverview());

      checkCalculateStatus = new Button(dialogArea, SWT.CHECK);
      checkCalculateStatus.setText(Messages.get().OtherOptions_UseForStatusCalculation);
      checkCalculateStatus.setSelection(dci.isUsedForNodeStatusCalculation());

      checkHideOnLastValues = new Button(dialogArea, SWT.CHECK);
      checkHideOnLastValues.setText("Hide value on \"Last Values\" page");
      checkHideOnLastValues.setSelection(dci.isHideOnLastValuesView());      

      agentCacheMode = new LabeledCombo(dialogArea, SWT.NONE);
      agentCacheMode.setLabel(Messages.get().General_AgentCacheMode);
      agentCacheMode.add(Messages.get().General_Default);
      agentCacheMode.add(Messages.get().General_On);
      agentCacheMode.add(Messages.get().General_Off);
      agentCacheMode.select(dci.getCacheMode().getValue());

      multiplierDegree = new LabeledCombo(dialogArea, SWT.NONE);
      multiplierDegree.setLabel("Multiplier degree");
      multiplierDegree.add("Fixed to P");
      multiplierDegree.add("Fixed to T");
      multiplierDegree.add("Fixed to G");
      multiplierDegree.add("Fixed to M");
      multiplierDegree.add("Fixed to K");
      multiplierDegree.add("Default");
      multiplierDegree.add("Fixed to m");
      multiplierDegree.add("Fixed to μ");
      multiplierDegree.add("Fixed to n");
      multiplierDegree.add("Fixed to p");
      multiplierDegree.add("Fixed to f");
      multiplierDegree.select(5 - dci.getMultiplier());

      relatedObject = new ObjectSelector(dialogArea, SWT.NONE, true);
      relatedObject.setLabel("Related object");
      relatedObject.setObjectClass(GenericObject.class);
      relatedObject.setObjectId(dci.getRelatedObject());
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      relatedObject.setLayoutData(gd);

      interpretation = new LabeledCombo(dialogArea, SWT.NONE);
      interpretation.setLabel("Interpretation");
      interpretation.add("Other");
      interpretation.add("Interface traffic - Inbound - bits/sec");
      interpretation.add("Interface traffic - Inbound - bytes/sec");
      interpretation.add("Interface traffic - Inbound - Utilization %");
      interpretation.add("Interface traffic - Outbound - bits/sec");
      interpretation.add("Interface traffic - Outbound - bytes/sec");
      interpretation.add("Interface traffic - Outbound - Utilization %");
      interpretation.add("Interface speed");
      interpretation.select(interpretationFromTag(dci.getSystemTag()));

      return dialogArea;
   }

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
   protected void applyChanges(final boolean isApply)
   {
		dci.setShowOnObjectTooltip(checkShowOnTooltip.getSelection());
		dci.setShowInObjectOverview(checkShowInOverview.getSelection());
      dci.setUsedForNodeStatusCalculation(checkCalculateStatus.getSelection());
      dci.setHideOnLastValuesView(checkHideOnLastValues.getSelection());
      dci.setCacheMode(AgentCacheMode.getByValue(agentCacheMode.getSelectionIndex()));
      dci.setMultiplier(5 - multiplierDegree.getSelectionIndex());
      dci.setRelatedObject(relatedObject.getObjectId());
      dci.setSystemTag(tagFromInterpretation(interpretation.getSelectionIndex()));
      editor.modify();
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      applyChanges(false);
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected void performApply()
   {
      applyChanges(true);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		checkShowOnTooltip.setSelection(false);
		checkShowInOverview.setSelection(false);
		checkCalculateStatus.setSelection(false);
		checkHideOnLastValues.setSelection(false);
		agentCacheMode.select(0);
		multiplierDegree.select(0);
		relatedObject.setObjectId(0);
      interpretation.select(0);
	}

   /**
    * Get interpretation code from DCI tag.
    *
    * @param tag DCI tag
    * @return interpretation code
    */
   private static int interpretationFromTag(String tag)
   {
      if ((tag == null) || tag.isEmpty())
         return 0;
      for(int i = 0; i < TAGS.length; i++)
         if (tag.equalsIgnoreCase(TAGS[i]))
            return i + 1;
      return 0;
   }

   /**
    * Get tag from interpretation code.
    *
    * @param index interpretation code
    * @return tag or empty string
    */
   private static String tagFromInterpretation(int index)
   {
      return ((index > 0) && (index <= TAGS.length)) ? TAGS[index - 1] : "";
   }
}
