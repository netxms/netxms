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

import java.time.ZoneId;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.fieldassist.ContentProposal;
import org.eclipse.jface.fieldassist.ContentProposalAdapter;
import org.eclipse.jface.fieldassist.IContentProposal;
import org.eclipse.jface.fieldassist.IContentProposalProvider;
import org.eclipse.jface.fieldassist.TextContentAdapter;
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
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectCategorySelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "General" property page for NetXMS objects
 */
public class General extends ObjectPropertyPage
{
   private static String[] zoneIds = null;

   private I18n i18n = LocalizationHelper.getI18n(General.class);

   private LabeledText name;
   private LabeledText alias;
   private LabeledText aiHint;
   private ObjectCategorySelector categorySelector;
   private LabeledText timeZone;
   private Button checkHidden;
	private String initialName;
   private String initialAlias;
   private String initialAiHint;
   private int initialCategory;
   private String initialTimeZone;
   private boolean initialHidden;

   /**
    * Get sorted list of IANA time zone IDs known to the JVM (region-based IDs plus "UTC").
    *
    * @return sorted array of time zone IDs
    */
   private static synchronized String[] getZoneIds()
   {
      if (zoneIds == null)
      {
         List<String> list = new ArrayList<>();
         for(String id : ZoneId.getAvailableZoneIds())
         {
            if (id.contains("/") || id.equals("UTC"))
               list.add(id);
         }
         Collections.sort(list);
         zoneIds = list.toArray(new String[list.size()]);
      }
      return zoneIds;
   }

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public General(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(General.class).tr("General"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "general";
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
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      // Object name
      initialName = object.getObjectName();
      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("Name"));
      name.setText(initialName);
      name.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Object alias
      initialAlias = object.getAlias();
      alias = new LabeledText(dialogArea, SWT.NONE);
      alias.setLabel(i18n.tr("Alias"));
      alias.setText(initialAlias);
      alias.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // AI hint
      initialAiHint = (object.getAiHint() != null) ? object.getAiHint() : "";
      aiHint = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI);
      aiHint.setLabel(i18n.tr("AI Hint"));
      aiHint.setText(initialAiHint);
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.heightHint = 160;
      aiHint.setLayoutData(gd);

      // Category selector
      initialCategory = object.getCategoryId();
      categorySelector = new ObjectCategorySelector(dialogArea, SWT.NONE);
      categorySelector.setLabel(i18n.tr("Category"));
      categorySelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      categorySelector.setCategoryId(initialCategory);

      // Time zone (own setting only; inherited zone is shown as a hint)
      initialTimeZone = ((object.getTimeZone() != null) && !object.isTimeZoneInherited()) ? object.getTimeZone() : "";
      timeZone = new LabeledText(dialogArea, SWT.NONE);
      timeZone.setLabel(i18n.tr("Time zone"));
      timeZone.setText(initialTimeZone);
      timeZone.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      ContentProposalAdapter proposalAdapter = new ContentProposalAdapter(timeZone.getTextControl(), new TextContentAdapter(), new IContentProposalProvider() {
         @Override
         public IContentProposal[] getProposals(String contents, int position)
         {
            String filter = contents.trim().toLowerCase();
            List<IContentProposal> proposals = new ArrayList<>();
            for(String id : getZoneIds())
            {
               if (id.toLowerCase().contains(filter))
                  proposals.add(new ContentProposal(id));
            }
            return proposals.toArray(new IContentProposal[proposals.size()]);
         }
      }, null, null);
      proposalAdapter.setProposalAcceptanceStyle(ContentProposalAdapter.PROPOSAL_REPLACE);
      proposalAdapter.setPropagateKeys(true);

      Label timeZoneHint = new Label(dialogArea, SWT.WRAP);
      if (object.isTimeZoneInherited())
         timeZoneHint.setText(String.format(i18n.tr("Inherited from %s: %s"), Registry.getSession().getObjectName(object.getTimeZoneSourceObjectId()), object.getTimeZone()));
      else
         timeZoneHint.setText(i18n.tr("Empty value inherits the time zone from parent objects"));
      timeZoneHint.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

      // Hidden checkbox
      initialHidden = object.isHidden();
      checkHidden = new Button(dialogArea, SWT.CHECK);
      checkHidden.setText(i18n.tr("Hidden"));
      checkHidden.setSelection(initialHidden);

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createControl(Composite parent)
   {
      super.createControl(parent);
      getDefaultsButton().setVisible(false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
      final String newName = name.getText();
      final String newAlias = alias.getText();
      final String newAiHint = aiHint.getText();
      final int newCategory = categorySelector.getCategoryId();
      final String newTimeZone = timeZone.getText().trim();
      final boolean newHidden = checkHidden.getSelection();
      if (newName.equals(initialName) && newAlias.equals(initialAlias) && newAiHint.equals(initialAiHint) && (newCategory == initialCategory) &&
          newTimeZone.equals(initialTimeZone) && (newHidden == initialHidden))
         return true; // nothing to change

		if (isApply)
			setValid(false);

      final NXCSession session = Registry.getSession();
		final NXCObjectModificationData data = new NXCObjectModificationData(object.getObjectId());
		data.setName(newName);
      data.setAlias(newAlias);
      data.setAiHint(newAiHint);
      data.setCategoryId(newCategory);
      if (!newTimeZone.equals(initialTimeZone))
         data.setTimeZone(newTimeZone);
      data.setHidden(newHidden);
      new Job(i18n.tr("Updating object properties"), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(data);
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot modify object");
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							initialName = newName;
                     initialAlias = newAlias;
                     initialAiHint = newAiHint;
                     initialCategory = newCategory;
                     initialTimeZone = newTimeZone;
                     initialHidden = newHidden;
							General.this.setValid(true);
						}
					});
				}
			}
		}.start();

      return true;
	}
}
