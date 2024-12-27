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
package org.netxms.nxmc.modules.datacollection.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.objects.GenericObject;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * DCI property page "Other Options"
 */
public class OtherOptions extends AbstractDCIPropertyPage
{
   private static final String[] TAGS = { "iface-inbound-bits", "iface-inbound-bytes", "iface-inbound-util", "iface-outbound-bits", "iface-outbound-bytes", "iface-outbound-util", "iface-speed" };

   private final I18n i18n = LocalizationHelper.getI18n(OtherOptions.class);

	private DataCollectionItem dci;
	private Button checkShowOnTooltip;
	private Button checkShowInOverview;
   private Button checkCalculateStatus;
   private Button checkHideOnLastValues;
   private LabeledCombo multiplierDegree;
   private LabeledCombo agentCacheMode;
   private LabeledCombo interpretation;
   private LabeledText userTag;
   private ObjectSelector relatedObject;

   /**
    * Constructor
    * 
    * @param editor
    */
   public OtherOptions(DataCollectionObjectEditor editor)
   {
      super(LocalizationHelper.getI18n(OtherOptions.class).tr("Other Options"), editor);
   }

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
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);

      checkShowOnTooltip = new Button(dialogArea, SWT.CHECK);
      checkShowOnTooltip.setText(i18n.tr("&Show last value in object tooltips and large labels on maps"));
      checkShowOnTooltip.setSelection(dci.isShowOnObjectTooltip());
      GridData gd = new GridData();
      gd.horizontalSpan = 2;
      checkShowOnTooltip.setLayoutData(gd);

      checkShowInOverview = new Button(dialogArea, SWT.CHECK);
      checkShowInOverview.setText(i18n.tr("Show last value in object &overview"));
      checkShowInOverview.setSelection(dci.isShowInObjectOverview());
      gd = new GridData();
      gd.horizontalSpan = 2;
      checkShowInOverview.setLayoutData(gd);

      checkCalculateStatus = new Button(dialogArea, SWT.CHECK);
      checkCalculateStatus.setText(i18n.tr("&Use this DCI for node status calculation"));
      checkCalculateStatus.setSelection(dci.isUsedForNodeStatusCalculation());
      gd = new GridData();
      gd.horizontalSpan = 2;
      checkCalculateStatus.setLayoutData(gd);

      checkHideOnLastValues = new Button(dialogArea, SWT.CHECK);
      checkHideOnLastValues.setText("&Hide value on \"Last Values\" page");
      checkHideOnLastValues.setSelection(dci.isHideOnLastValuesView());      
      gd = new GridData();
      gd.horizontalSpan = 2;
      checkHideOnLastValues.setLayoutData(gd);

      agentCacheMode = new LabeledCombo(dialogArea, SWT.NONE);
      agentCacheMode.setLabel(i18n.tr("Agent cache mode"));
      agentCacheMode.add(i18n.tr("Default"));
      agentCacheMode.add(i18n.tr("On"));
      agentCacheMode.add(i18n.tr("Off"));
      agentCacheMode.select(dci.getCacheMode().getValue());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      agentCacheMode.setLayoutData(gd);

      multiplierDegree = new LabeledCombo(dialogArea, SWT.NONE);
      multiplierDegree.setLabel(i18n.tr("Multiplier degree"));
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
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      multiplierDegree.setLayoutData(gd);

      relatedObject = new ObjectSelector(dialogArea, SWT.NONE, true);
      relatedObject.setLabel("Related object");
      relatedObject.setObjectClass(GenericObject.class);
      relatedObject.setObjectId(dci.getRelatedObject());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      relatedObject.setLayoutData(gd);

      interpretation = new LabeledCombo(dialogArea, SWT.NONE);
      interpretation.setLabel(i18n.tr("Interpretation"));
      interpretation.add(i18n.tr("Other"));
      interpretation.add(i18n.tr("Interface traffic - Inbound - bits/sec"));
      interpretation.add(i18n.tr("Interface traffic - Inbound - bytes/sec"));
      interpretation.add(i18n.tr("Interface traffic - Inbound - Utilization %"));
      interpretation.add(i18n.tr("Interface traffic - Outbound - bits/sec"));
      interpretation.add(i18n.tr("Interface traffic - Outbound - bytes/sec"));
      interpretation.add(i18n.tr("Interface traffic - Outbound - Utilization %"));
      interpretation.add(i18n.tr("Interface speed"));
      interpretation.select(interpretationFromTag(dci.getSystemTag()));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      interpretation.setLayoutData(gd);

      userTag = new LabeledText(dialogArea, SWT.NONE);
      userTag.setLabel(i18n.tr("Tag"));
      userTag.setText(dci.getUserTag());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      userTag.setLayoutData(gd);

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
		dci.setShowOnObjectTooltip(checkShowOnTooltip.getSelection());
		dci.setShowInObjectOverview(checkShowInOverview.getSelection());
      dci.setUsedForNodeStatusCalculation(checkCalculateStatus.getSelection());
      dci.setHideOnLastValuesView(checkHideOnLastValues.getSelection());
      dci.setCacheMode(AgentCacheMode.getByValue(agentCacheMode.getSelectionIndex()));
      dci.setMultiplier(5 - multiplierDegree.getSelectionIndex());
      dci.setRelatedObject(relatedObject.getObjectId());
      dci.setSystemTag(tagFromInterpretation(interpretation.getSelectionIndex()));
      dci.setUserTag(userTag.getText().trim());
      editor.modify();
      return true;
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
      userTag.setText("");
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
