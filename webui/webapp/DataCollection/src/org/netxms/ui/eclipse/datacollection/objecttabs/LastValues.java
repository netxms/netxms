/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.objecttabs;

import org.eclipse.core.commands.Command;
import org.eclipse.core.commands.State;
import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.commands.ICommandService;
import org.eclipse.ui.contexts.IContextService;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.datacollection.widgets.LastValuesWidget;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;

/**
 * Last values tab
 */
public class LastValues extends ObjectTab
{
	private LastValuesWidget dataView;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		dataView = new LastValuesWidget(getViewPart(), parent, SWT.NONE, (Node)getObject(), "LastValuesTab"); //$NON-NLS-1$
		dataView.setAutoRefreshEnabled(true);
		dataView.setFilterCloseAction(new Action() {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				dataView.enableFilter(false);
				ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);
				Command command = service.getCommand("org.netxms.ui.eclipse.datacollection.commands.show_dci_filter"); //$NON-NLS-1$
				State state = command.getState("org.netxms.ui.eclipse.datacollection.commands.show_dci_filter.state"); //$NON-NLS-1$
				state.setValue(false);
				service.refreshElements(command.getId(), null);
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.GenericObject)
	 */
	@Override
	public void objectChanged(GenericObject object)
	{
		dataView.setNode(object);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.GenericObject)
	 */
	@Override
	public boolean showForObject(GenericObject object)
	{
		return (object instanceof Node) || (object instanceof MobileDevice);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
	 */
	@Override
	public void refresh()
	{
		dataView.refresh();
	}

	/**
	 * @param enabled
	 */
	public void setFilterEnabled(boolean enabled)
	{
		dataView.enableFilter(enabled);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#selected()
	 */
	@Override
	public void selected()
	{
		super.selected();
		IContextService contextService = (IContextService)getViewPart().getSite().getService(IContextService.class);
		if (contextService != null)
		{
			contextService.activateContext("org.netxms.ui.eclipse.datacollection.context.LastValues"); //$NON-NLS-1$
		}
	}
}
