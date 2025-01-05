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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.client.objects.Template;
import org.netxms.client.objects.TemplateGroup;
import org.netxms.client.objects.TemplateRoot;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.imagelibrary.widgets.ImageSelector;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Map Appearance" property page for NetMS objects 
 */
public class MapAppearance extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(MapAppearance.class);

   private LabeledText nameOnMap;
	private ImageSelector image;
	private ObjectSelector drillDownObject = null;
	
   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public MapAppearance(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(MapAppearance.class).tr("Map Appearance"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "map-appearance";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return !((object instanceof Template) || (object instanceof TemplateGroup) || (object instanceof TemplateRoot));
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

      // Name on maps
      nameOnMap = new LabeledText(dialogArea, SWT.NONE);
      nameOnMap.setLabel("Name on network maps (leave empty to use normal object name)");
      nameOnMap.setText(object.getConfiguredNameOnMap());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      nameOnMap.setLayoutData(gd);

      // Image
      image = new ImageSelector(dialogArea, SWT.NONE);
      image.setLabel(i18n.tr("Presentation image"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      image.setLayoutData(gd);
      image.setImageGuid(object.getMapImage(), false);
      
      // Submap
      if (!(object instanceof NetworkMap))
      {
         drillDownObject = new ObjectSelector(dialogArea, SWT.NONE, true);
         drillDownObject.setLabel("Drill-down object");
         drillDownObject.setObjectClass(AbstractObject.class);
         drillDownObject.setObjectId(object.getDrillDownObjectId());
         drillDownObject.setClassFilter(ObjectSelectionDialog.createDashboardAndNetworkMapSelectionFilter());
	      gd = new GridData();
	      gd.horizontalAlignment = SWT.FILL;
	      gd.grabExcessHorizontalSpace = true;
	      drillDownObject.setLayoutData(gd);
      }
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
		
		final NXCObjectModificationData data = new NXCObjectModificationData(object.getObjectId());
      data.setNameOnMap(nameOnMap.getText().trim());
      data.setMapImage(image.getImageGuid());
		if (drillDownObject != null)
			data.setDrillDownObjectId(drillDownObject.getObjectId());
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating object's map appearance"), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(data);
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format(i18n.tr("Cannot modify object %s"), object.getObjectName());
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
							MapAppearance.this.setValid(true);
						}
					});
				}
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
      nameOnMap.setText("");
      image.setImageGuid(NXCommon.EMPTY_GUID, true);
      if (!(object instanceof NetworkMap))
         drillDownObject.setObjectId(0);
   }
}
