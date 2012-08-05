/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.epp.dialogs;

import java.util.Collection;
import java.util.List;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.ServerAction;
import org.netxms.ui.eclipse.epp.Activator;
import org.netxms.ui.eclipse.epp.Messages;
import org.netxms.ui.eclipse.epp.dialogs.helpers.ActionComparator;
import org.netxms.ui.eclipse.epp.dialogs.helpers.ActionListFilter;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Action selection dialog
 *
 */
public class ActionSelectionDialog extends Dialog
{
	private boolean multiSelection;
	private Text filterText;
	private TableViewer eventList;
	private Collection<ServerAction> actions;
	private ServerAction selectedActions[];
	private ActionListFilter filter;

	/**
	 * @param parentShell
	 */
	public ActionSelectionDialog(Shell parentShell, Collection<ServerAction> actions)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		this.actions = actions;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.ActionSelectionDialog_Title);
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		try
		{
			newShell.setSize(settings.getInt("SelectAction.cx"), settings.getInt("SelectAction.cy")); //$NON-NLS-1$ //$NON-NLS-2$
		}
		catch(NumberFormatException e)
		{
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		new Label(dialogArea, SWT.NONE).setText(Messages.ActionSelectionDialog_Filter);
		
		filterText = new Text(dialogArea, SWT.NONE);
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		filterText.setLayoutData(gd);
		final String filterString = settings.get("SelectAction.Filter"); //$NON-NLS-1$
		if (filterString != null)
			filterText.setText(filterString);
		
		eventList = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION | (multiSelection ? SWT.MULTI : SWT.SINGLE) | SWT.H_SCROLL | SWT.V_SCROLL);
		eventList.setContentProvider(new ArrayContentProvider());
		eventList.setComparator(new ActionComparator());
		eventList.setLabelProvider(new WorkbenchLabelProvider());
		filter = new ActionListFilter();
		if (filterString != null)
			filter.setFilterString(filterString);
		eventList.addFilter(filter);
		eventList.setInput(actions.toArray());
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalSpan = 2;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 350;
		eventList.getTable().setLayoutData(gd);
		
		filterText.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				filter.setFilterString(filterText.getText());
				eventList.refresh();
			}
		});
		
		eventList.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				ActionSelectionDialog.this.okPressed();
			}
		});
		
		return dialogArea;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
	 */
	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@SuppressWarnings("unchecked")
	@Override
	protected void okPressed()
	{
		final IStructuredSelection selection = (IStructuredSelection)eventList.getSelection();
		final List<ServerAction> list = selection.toList();
		selectedActions = list.toArray(new ServerAction[list.size()]);
		saveSettings();
		super.okPressed();
	}
	
	/**
	 * Save dialog settings
	 */
	private void saveSettings()
	{
		Point size = getShell().getSize();
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		settings.put("SelectAction.cx", size.x); //$NON-NLS-1$
		settings.put("SelectAction.cy", size.y); //$NON-NLS-1$
		settings.put("SelectAction.Filter", filterText.getText()); //$NON-NLS-1$
	}

	/**
	 * @return true if multiple event selection is enabled
	 */
	public boolean isMultiSelectionEnabled()
	{
		return multiSelection;
	}

	/**
	 * Enable or disable selection of multiple events.
	 * 
	 * @param enable true to enable multiselection, false to disable
	 */
	public void enableMultiSelection(boolean enable)
	{
		this.multiSelection = enable;
	}

	/**
	 * Get selected event templates
	 * 
	 * @return Selected event templates
	 */
	public ServerAction[] getSelectedActions()
	{
		return selectedActions;
	}
}
