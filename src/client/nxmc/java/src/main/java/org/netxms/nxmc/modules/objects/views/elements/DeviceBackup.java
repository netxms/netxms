/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views.elements;

import org.eclipse.swt.widgets.Composite;
import org.netxms.client.constants.DeviceBackupJobStatus;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.xnap.commons.i18n.I18n;

/**
 * "Device Backup" element for object overview page
 */
public class DeviceBackup extends TableElement
{
   private final I18n i18n = LocalizationHelper.getI18n(DeviceBackup.class);

   private boolean enabled = Registry.getSession().isServerComponentRegistered("DEVBACKUP");

	/**
	 * @param parent
	 * @param anchor
	 * @param objectView
	 */
   public DeviceBackup(Composite parent, OverviewPageElement anchor, ObjectView objectView)
	{
		super(parent, anchor, objectView);
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
	@Override
	protected String getTitle()
	{
      return i18n.tr("Device Backup");
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.TableElement#fillTable()
    */
	@Override
	protected void fillTable()
	{
		if (!(getObject() instanceof AbstractNode))
			return;

		AbstractNode node = (AbstractNode)getObject();
      addPair(i18n.tr("Registered"), node.isRegisteredForConfigBackup() ? i18n.tr("Yes") : i18n.tr("No"));
      addPair(i18n.tr("Last job status"), jobStatusToText(node.getLastConfigBackupJobStatus()));
	}

   /**
    * Convert job status to text.
    *
    * @param status job status
    * @return text representation
    */
   private String jobStatusToText(DeviceBackupJobStatus status)
	{
	   switch(status)
      {
         case FAILED:
            return i18n.tr("Failed");
         case SUCCESSFUL:
            return i18n.tr("Successful");
         default:
            return i18n.tr("Unknown");
      }
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
	@Override
	public boolean isApplicableForObject(AbstractObject object)
	{
      return enabled && (object instanceof AbstractNode);
	}
}
