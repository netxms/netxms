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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Object Context" property page for network map object
 */
public class MapObjectContext extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(MapObjectContext.class);

   private NetworkMap map;
   private Button checkboxShowAsObjectView;
   private LabeledSpinner displayPriority;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public MapObjectContext(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(MapObjectContext.class).tr("Object Context"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "map-object-context";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return object instanceof NetworkMap;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 20;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = new Composite(parent, SWT.NONE);
		
      map = (NetworkMap)object;

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      checkboxShowAsObjectView = new Button(dialogArea, SWT.CHECK);
      checkboxShowAsObjectView.setText(i18n.tr("&Automatically show this map in object context"));
      checkboxShowAsObjectView.setSelection((map.getFlags() & NetworkMap.MF_SHOW_AS_OBJECT_VIEW) != 0);
      checkboxShowAsObjectView.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false));

      displayPriority = new LabeledSpinner(dialogArea, SWT.NONE);
      displayPriority.setLabel(i18n.tr("Display priority (1-65535, 0 for automatic)"));
      displayPriority.setRange(0, 65535);
      displayPriority.setSelection(map.getDisplayPriority());
      displayPriority.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false));

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
		if (isApply)
			setValid(false);

      int flags = 0;
      if (checkboxShowAsObjectView.getSelection())
         flags |= NetworkMap.MF_SHOW_AS_OBJECT_VIEW;

		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      md.setObjectFlags(flags, NetworkMap.MF_SHOW_AS_OBJECT_VIEW);
      md.setDisplayPriority(displayPriority.getSelection());

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating network map object context configuration"), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> MapObjectContext.this.setValid(true));
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot update network map object context configuration");
			}
		}.start();

      return true;
	}
}
