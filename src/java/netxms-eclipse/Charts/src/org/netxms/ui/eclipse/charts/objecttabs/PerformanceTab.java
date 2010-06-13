/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.charts.objecttabs;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.PerfTabDci;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * Performance tab
 *
 */
public class PerformanceTab extends ObjectTab
{
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		parent.setLayout(layout);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.GenericObject)
	 */
	@Override
	public void objectChanged(GenericObject object)
	{
		update(object);
	}
	
	/**
	 * Update tab with object's data
	 * 
	 * @param object New object
	 */
	private void update(final GenericObject object)
	{
		Job job = new Job("Update performance tab") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				final NXCSession session = NXMCSharedData.getInstance().getSession();
				try
				{
					final PerfTabDci[] items = session.getPerfTabItems(object.getObjectId());
					new UIJob("Update performance tab") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							if (PerformanceTab.this.getObject().getObjectId() == object.getObjectId())
							{
								
							}
							return Status.OK_STATUS;
						}
					}.schedule();
				}
				catch(Exception e)
				{
				}
				return Status.OK_STATUS;
			}
		};
		job.setSystem(true);
		job.schedule();
	}
}
