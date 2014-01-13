/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.actions;

import java.util.Map;
import org.eclipse.core.commands.AbstractHandler;
import org.eclipse.core.commands.Command;
import org.eclipse.core.commands.ExecutionEvent;
import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.commands.State;
import org.eclipse.core.expressions.IEvaluationContext;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.commands.ICommandService;
import org.eclipse.ui.commands.IElementUpdater;
import org.eclipse.ui.menus.UIElement;
import org.netxms.ui.eclipse.datacollection.objecttabs.LastValues;

/**
 * Command handler for toggling DCI filter state
 */
public class ToggleDCIFilterHandler extends AbstractHandler implements IElementUpdater
{
	/* (non-Javadoc)
	 * @see org.eclipse.core.commands.AbstractHandler#execute(org.eclipse.core.commands.ExecutionEvent)
	 */
	@Override
	public Object execute(ExecutionEvent event) throws ExecutionException
	{
		Object object = event.getApplicationContext();
		if (object instanceof IEvaluationContext)
		{
			Object tab = ((IEvaluationContext)object).getVariable("org.netxms.ui.eclipse.objectview.ActiveTab");
			if ((tab != null) && (tab instanceof LastValues))
			{
				ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);
				Command command = service.getCommand("org.netxms.ui.eclipse.datacollection.commands.show_dci_filter"); //$NON-NLS-1$
				State state = command.getState("org.netxms.ui.eclipse.datacollection.commands.show_dci_filter.state"); //$NON-NLS-1$
				boolean isChecked = !(Boolean)state.getValue();
				state.setValue(isChecked);
				((LastValues)tab).setFilterEnabled(isChecked);
				service.refreshElements(event.getCommand().getId(), null);
			}
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.commands.IElementUpdater#updateElement(org.eclipse.ui.menus.UIElement, java.util.Map)
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public void updateElement(UIElement element, Map parameters)
	{
		ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);
		Command command = service.getCommand("org.netxms.ui.eclipse.datacollection.commands.show_dci_filter"); //$NON-NLS-1$
		State state = command.getState("org.netxms.ui.eclipse.datacollection.commands.show_dci_filter.state"); //$NON-NLS-1$
		element.setChecked((Boolean)state.getValue());
	}
}
