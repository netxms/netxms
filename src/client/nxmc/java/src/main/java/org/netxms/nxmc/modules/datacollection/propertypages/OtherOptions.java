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
package org.netxms.nxmc.modules.datacollection.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
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
   /**
    * System tags offered by the interpretation selector, in display order, as pairs of tag and display label. Labels are marked
    * for translation here and translated when shown. Tags not in this list (retired tags such as "eed.ren", or tags set via API)
    * are still shown and preserved as entered.
    */
   public static final String[][] TAGS = {
      { "iface.inbound.bits", I18n.marktr("Interface traffic - Inbound - bits/sec") },
      { "iface.inbound.bytes", I18n.marktr("Interface traffic - Inbound - bytes/sec") },
      { "iface.inbound.util", I18n.marktr("Interface traffic - Inbound - Utilization %") },
      { "iface.outbound.bits", I18n.marktr("Interface traffic - Outbound - bits/sec") },
      { "iface.outbound.bytes", I18n.marktr("Interface traffic - Outbound - bytes/sec") },
      { "iface.outbound.util", I18n.marktr("Interface traffic - Outbound - Utilization %") },
      { "iface.speed", I18n.marktr("Interface speed") },
      { "eed.edc", I18n.marktr("EED - Total facility energy (EDC)") },
      { "eed.edc_bg", I18n.marktr("EED - Back-up generator energy (EDC_BG)") },
      { "eed.res_os", I18n.marktr("EED - On-site renewable energy (RES_OS)") },
      { "eed.eit", I18n.marktr("EED - IT equipment energy (EIT)") },
      { "eed.win", I18n.marktr("EED - Water intake (WIN)") },
      { "eed.win_fre", I18n.marktr("EED - Freshwater intake (WIN_FRE)") },
      { "eed.reuse", I18n.marktr("EED - Reused energy (REUSE)") },
      { "eed.reuse_upgrade", I18n.marktr("EED - Heat upgrade energy (HU)") },
      { "power.feed", I18n.marktr("Power - Feed power") },
      { "power.in", I18n.marktr("Power - Input side power") },
      { "power.out", I18n.marktr("Power - Output side power") },
      { "power.gen", I18n.marktr("Power - On-site generation") },
      { "cooling.energy", I18n.marktr("Cooling - Energy consumption") },
      { "cooling.power", I18n.marktr("Cooling - Power consumption") },
      { "cooling.load", I18n.marktr("Cooling - Thermal load") },
      { "cooling.temp.supply", I18n.marktr("Cooling - Supply temperature") },
      { "cooling.temp.return", I18n.marktr("Cooling - Return temperature") },
      { "cooling.flow", I18n.marktr("Cooling - Coolant flow rate") },
      { "env.temp.outdoor", I18n.marktr("Environment - Outdoor temperature") },
      { "env.temp.ambient", I18n.marktr("Environment - Ambient temperature") },
      { "env.temp.supply", I18n.marktr("Environment - Supply air temperature") },
      { "env.temp.return", I18n.marktr("Environment - Return air temperature") },
      { "env.temp.inlet", I18n.marktr("Environment - Rack inlet temperature") },
      { "env.temp.outlet", I18n.marktr("Environment - Rack outlet temperature") },
      { "env.humidity.outdoor", I18n.marktr("Environment - Outdoor humidity") },
      { "env.humidity.ambient", I18n.marktr("Environment - Ambient humidity") },
      { "water.flow", I18n.marktr("Water - Intake flow rate") },
      { "heat.export", I18n.marktr("Heat - Exported heat flow") }
   };

   private final I18n i18n = LocalizationHelper.getI18n(OtherOptions.class);

	private DataCollectionItem dci;
	private Button checkShowOnTooltip;
	private Button checkShowInOverview;
   private Button checkCalculateStatus;
   private Button checkHideInViewMode;
   private Button checkUnsupportedAsError;
   private LabeledCombo multiplierDegree;
   private LabeledCombo agentCacheMode;
   private LabeledCombo interpretation;
   private LabeledText feedQualifier;
   private String unlistedTag = null; // base tag of the DCI if it is not in TAGS; preserved on save
   private LabeledText userTag;
   private ObjectSelector relatedObject;
   private LabeledText aiHint;

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

      checkHideInViewMode = new Button(dialogArea, SWT.CHECK);
      checkHideInViewMode.setText(i18n.tr("&Hide in view mode"));
      checkHideInViewMode.setSelection(dci.isHideOnLastValuesView());      
      gd = new GridData();
      gd.horizontalSpan = 2;
      checkHideInViewMode.setLayoutData(gd);

      checkUnsupportedAsError = new Button(dialogArea, SWT.CHECK);
      checkUnsupportedAsError.setText(i18n.tr("Interpret &unsupported state as data collection error"));
      checkUnsupportedAsError.setSelection(dci.isUnsupportedAsError());
      gd = new GridData();
      gd.horizontalSpan = 2;
      checkUnsupportedAsError.setLayoutData(gd);

      agentCacheMode = new LabeledCombo(dialogArea, SWT.NONE);
      agentCacheMode.setLabel(i18n.tr("Agent cache mode"));
      agentCacheMode.add(i18n.tr("Default"));
      agentCacheMode.add(i18n.tr("On"));
      agentCacheMode.add(i18n.tr("Off"));
      agentCacheMode.select(dci.getCacheMode().getValue());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      agentCacheMode.setLayoutData(gd);

      multiplierDegree = new LabeledCombo(dialogArea, SWT.NONE);
      multiplierDegree.setLabel(i18n.tr("Multiplier degree"));
      multiplierDegree.add(i18n.tr("Fixed to P"));
      multiplierDegree.add(i18n.tr("Fixed to T"));
      multiplierDegree.add(i18n.tr("Fixed to G"));
      multiplierDegree.add(i18n.tr("Fixed to M"));
      multiplierDegree.add(i18n.tr("Fixed to K"));
      multiplierDegree.add("Default");
      multiplierDegree.add(i18n.tr("Fixed to m"));
      multiplierDegree.add(i18n.tr("Fixed to μ"));
      multiplierDegree.add(i18n.tr("Fixed to n"));
      multiplierDegree.add(i18n.tr("Fixed to p"));
      multiplierDegree.add(i18n.tr("Fixed to f"));
      multiplierDegree.select(5 - dci.getMultiplier());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      multiplierDegree.setLayoutData(gd);

      relatedObject = new ObjectSelector(dialogArea, SWT.NONE, true);
      relatedObject.setLabel(i18n.tr("Related object"));
      relatedObject.setObjectClass(GenericObject.class);
      relatedObject.setObjectId(dci.getRelatedObject());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      relatedObject.setLayoutData(gd);

      String systemTag = (dci.getSystemTag() != null) ? dci.getSystemTag() : "";
      int separator = systemTag.indexOf('/');
      String baseTag = (separator >= 0) ? systemTag.substring(0, separator) : systemTag;
      int tagIndex = interpretationFromTag(baseTag);
      if ((tagIndex == 0) && !baseTag.isEmpty())
      {
         unlistedTag = baseTag;
         tagIndex = TAGS.length + 1;
      }

      Composite interpretationArea = new Composite(dialogArea, SWT.NONE);
      GridLayout interpretationAreaLayout = new GridLayout();
      interpretationAreaLayout.numColumns = 2;
      interpretationAreaLayout.marginHeight = 0;
      interpretationAreaLayout.marginWidth = 0;
      interpretationArea.setLayout(interpretationAreaLayout);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      interpretationArea.setLayoutData(gd);

      interpretation = new LabeledCombo(interpretationArea, SWT.NONE);
      interpretation.setLabel(i18n.tr("Interpretation"));
      interpretation.add(i18n.tr("Other"));
      for(String[] entry : TAGS)
         interpretation.add(i18n.tr(entry[1]));
      if (unlistedTag != null)
         interpretation.add(unlistedTag);
      interpretation.select(tagIndex);
      interpretation.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            feedQualifier.setEnabled(interpretation.getSelectionIndex() > 0);
         }
      });
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      interpretation.setLayoutData(gd);

      feedQualifier = new LabeledText(interpretationArea, SWT.NONE);
      feedQualifier.setLabel(i18n.tr("Feed qualifier"));
      feedQualifier.getTextControl().setTextLimit(15);
      feedQualifier.setText((separator >= 0) ? systemTag.substring(separator + 1) : "");
      feedQualifier.setEnabled(tagIndex > 0);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      feedQualifier.setLayoutData(gd);

      userTag = new LabeledText(dialogArea, SWT.NONE);
      userTag.setLabel(i18n.tr("Tag"));
      userTag.setText(dci.getUserTag());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      userTag.setLayoutData(gd);

      aiHint = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI);
      aiHint.setLabel(i18n.tr("AI Hint"));
      aiHint.setText(dci.getAiHint());
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.heightHint = 160;
      gd.horizontalSpan = 2;
      aiHint.setLayoutData(gd);

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
      dci.setHideOnLastValuesView(checkHideInViewMode.getSelection());
      dci.setUnsupportedAsError(checkUnsupportedAsError.getSelection());
      dci.setCacheMode(AgentCacheMode.getByValue(agentCacheMode.getSelectionIndex()));
      dci.setMultiplier(5 - multiplierDegree.getSelectionIndex());
      dci.setRelatedObject(relatedObject.getObjectId());
      String tag = tagFromInterpretation(interpretation.getSelectionIndex());
      String qualifier = feedQualifier.getText().trim();
      dci.setSystemTag((tag.isEmpty() || qualifier.isEmpty()) ? tag : tag + "/" + qualifier);
      dci.setUserTag(userTag.getText().trim());
      dci.setAiHint(aiHint.getText().trim());
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
		checkHideInViewMode.setSelection(false);
      checkUnsupportedAsError.setSelection(false);
		agentCacheMode.select(0);
		multiplierDegree.select(0);
		relatedObject.setObjectId(0);
      interpretation.select(0);
      feedQualifier.setText("");
      feedQualifier.setEnabled(false);
      userTag.setText("");
      aiHint.setText("");
	}

   /**
    * Get interpretation selector index from base DCI tag (without qualifier).
    *
    * @param tag base DCI tag
    * @return index in selector, or 0 if tag is not in TAGS
    */
   private static int interpretationFromTag(String tag)
   {
      for(int i = 0; i < TAGS.length; i++)
         if (tag.equalsIgnoreCase(TAGS[i][0]))
            return i + 1;
      return 0;
   }

   /**
    * Get base DCI tag from interpretation selector index.
    *
    * @param index index in selector
    * @return tag or empty string
    */
   private String tagFromInterpretation(int index)
   {
      if ((index > 0) && (index <= TAGS.length))
         return TAGS[index - 1][0];
      if ((index == TAGS.length + 1) && (unlistedTag != null))
         return unlistedTag;
      return "";
   }
}
