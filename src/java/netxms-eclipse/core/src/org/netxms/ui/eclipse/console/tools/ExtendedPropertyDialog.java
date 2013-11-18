/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console.tools;

import java.util.Iterator;
import java.util.List;
import org.eclipse.jface.preference.IPreferenceNode;
import org.eclipse.jface.preference.IPreferencePage;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.osgi.util.NLS;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.internal.IWorkbenchHelpContextIds;
import org.eclipse.ui.internal.WorkbenchMessages;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.eclipse.ui.internal.dialogs.PropertyPageContributorManager;
import org.eclipse.ui.internal.dialogs.PropertyPageManager;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * @author Victor
 *
 */
@SuppressWarnings("restriction")
public class ExtendedPropertyDialog extends PropertyDialog
{
	/**
	 * @param parentShell
	 * @param mng
	 * @param selection
	 */
	public ExtendedPropertyDialog(Shell parentShell, PreferenceManager mng, ISelection selection)
	{
		super(parentShell, mng, selection);
	}
	
	/**
	 * Create controls for all pages
	 */
	public void createAllPages()
	{
		List<?> nodes = getPreferenceManager().getElements(PreferenceManager.POST_ORDER);
		Iterator<?> i = nodes.iterator();
		while(i.hasNext()) 
		{
			IPreferenceNode node = (IPreferenceNode)i.next();
			if (node.getPage() == null)
				createPage(node);
			IPreferencePage page = getPage(node);
			page.setContainer(this);
			if (page.getControl() == null)
				page.createControl(getPageContainer());
		}
	}

	/**
	 * @param shell
	 * @param propertyPageId
	 * @param element
	 * @param name
	 * @return
	 */
	public static ExtendedPropertyDialog createDialogOn(Shell shell, final String propertyPageId, Object element, String name)
	{
		if (element == null)
			return null;
		
		PropertyPageManager pageManager = new PropertyPageManager();
		String title = "";//$NON-NLS-1$
		
		// load pages for the selection
		// fill the manager with contributions from the matching contributors
		PropertyPageContributorManager.getManager().contribute(pageManager, element);
		// testing if there are pages in the manager
		Iterator<?> pages = pageManager.getElements(PreferenceManager.PRE_ORDER).iterator();
		if (!pages.hasNext())
		{
			MessageDialogHelper.openInformation(shell, WorkbenchMessages.PropertyDialog_messageTitle,
					NLS.bind(WorkbenchMessages.PropertyDialog_noPropertyMessage, name));
			return null;
		}
		title = NLS.bind(WorkbenchMessages.PropertyDialog_propertyMessage, name);
		ExtendedPropertyDialog propertyDialog = new ExtendedPropertyDialog(shell, pageManager, new StructuredSelection(element));

		if (propertyPageId != null)
		{
			propertyDialog.setSelectedNode(propertyPageId);
		}
		propertyDialog.create();

		propertyDialog.getShell().setText(title);
		PlatformUI.getWorkbench().getHelpSystem().setHelp(propertyDialog.getShell(), IWorkbenchHelpContextIds.PROPERTY_DIALOG);

		return propertyDialog;
	}
}
