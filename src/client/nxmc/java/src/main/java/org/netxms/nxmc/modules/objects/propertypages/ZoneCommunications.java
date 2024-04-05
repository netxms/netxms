/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ObjectList;
import org.xnap.commons.i18n.I18n;

/**
 * "Communications" property page for zone objects
 */
public class ZoneCommunications extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(ZoneCommunications.class);

	private Zone zone;
   private ObjectList proxyNodes = null;
   private boolean isModified = false;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public ZoneCommunications(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(ZoneCommunications.class).tr("Communications"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "zoneCommunications";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 10;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof Zone);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      zone = (Zone)object;

		Composite dialogArea = new Composite(parent, SWT.NONE);		
      dialogArea.setLayout(new FillLayout());   

      proxyNodes = new ObjectList(dialogArea, SWT.NONE, i18n.tr("Proxy nodes"), 
            zone.getProxyNodes(), AbstractNode.class, ObjectSelectionDialog.createNodeSelectionFilter(false), new Runnable() {
         @Override
         public void run()
         {
            isModified = true;
         }
      });

		return dialogArea;
	}

	/**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
	protected boolean applyChanges(final boolean isApply)
	{
      if (!isModified)
         return true;     // Nothing to apply

		if (isApply)
			setValid(false);

		final NXCObjectModificationData md = new NXCObjectModificationData(zone.getObjectId());
      md.setZoneProxies(proxyNodes.getObjectIdentifiers());

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating communication settings for zone {0}", zone.getObjectName()), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot update zone communication settings");
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> ZoneCommunications.this.setValid(true));
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
      isModified = true;
		proxyNodes.clear();
	}
}
