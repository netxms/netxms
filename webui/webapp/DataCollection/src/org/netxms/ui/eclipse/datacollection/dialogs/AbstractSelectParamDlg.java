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
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.AgentParameter;
import org.netxms.client.AgentTable;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.AgentParameterComparator;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.AgentParameterFilter;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.AgentParameterLabelProvider;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.AgentTableComparator;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.AgentTableLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Abstract base class for metric selection dialogs.
 */
public abstract class AbstractSelectParamDlg extends Dialog implements IParameterSelectionDialog
{
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_TYPE = 1;
	public static final int COLUMN_DESCRIPTION = 2;
	
	protected boolean selectTables;
	protected AbstractObject object;
	protected Text filterText;
	protected SortableTableViewer viewer;
	
	private Object selection;
	private AgentParameterFilter filter;

	/**
	 * @param parentShell
	 * @param nodeId
	 */
	public AbstractSelectParamDlg(Shell parentShell, long nodeId, boolean selectTables)
	{
		super(parentShell);
		this.selectTables = selectTables;
		setShellStyle(getShellStyle() | SWT.RESIZE);
		object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(nodeId);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(selectTables ? Messages.AbstractSelectParamDlg_TableSelection : Messages.AbstractSelectParamDlg_Title);
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		final String prefix = getConfigurationPrefix();
		try
		{
			newShell.setSize(settings.getInt(prefix + ".cx"), settings.getInt(prefix + ".cy")); //$NON-NLS-1$ //$NON-NLS-2$
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
		Composite dialogArea = (Composite)super.createDialogArea(parent);
	
		GridLayout layout = new GridLayout();
	   layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
	   layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
	   layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
	   dialogArea.setLayout(layout);
	   
	   Label label = new Label(dialogArea, SWT.NONE);
	   label.setText(selectTables ? Messages.AbstractSelectParamDlg_AvailTables : Messages.AbstractSelectParamDlg_AvailableParameters);
	   
	   filterText = new Text(dialogArea, SWT.BORDER);
	   GridData gd = new GridData();
	   gd.horizontalAlignment = SWT.FILL;
	   gd.grabExcessHorizontalSpace = true;
	   filterText.setLayoutData(gd);
	   filterText.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				filter.setFilter(filterText.getText());
				viewer.refresh(false);
			}
	   });
		
		final String[] names = { Messages.AbstractSelectParamDlg_Name, selectTables ? Messages.AbstractSelectParamDlg_InstanceColumn : Messages.AbstractSelectParamDlg_Type, Messages.AbstractSelectParamDlg_Description };
		final int[] widths = { 150, selectTables ? 150 : 100, 350 };
	   viewer = new SortableTableViewer(dialogArea, names, widths, 0, SWT.UP, SWT.FULL_SELECTION | SWT.BORDER);
	   WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), getConfigurationPrefix() + ".viewer"); //$NON-NLS-1$
	   viewer.setContentProvider(new ArrayContentProvider());
	   viewer.setLabelProvider(selectTables ? new AgentTableLabelProvider() : new AgentParameterLabelProvider());
	   viewer.setComparator(selectTables ? new AgentTableComparator() : new AgentParameterComparator());
	   
	   filter = new AgentParameterFilter();
	   viewer.addFilter(filter);
	   
	   viewer.getTable().addMouseListener(new MouseListener() {
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
				okPressed();
			}

			@Override
			public void mouseDown(MouseEvent e)
			{
			}

			@Override
			public void mouseUp(MouseEvent e)
			{
			}
	   });
	   
	   viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
			   WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), getConfigurationPrefix() + ".viewer"); //$NON-NLS-1$
			}
	   });

	   createPopupMenu();
	   
	   gd = new GridData();
	   gd.heightHint = 250;
	   gd.grabExcessVerticalSpace = true;
	   gd.verticalAlignment = SWT.FILL;
	   gd.grabExcessHorizontalSpace = true;
	   gd.horizontalAlignment = SWT.FILL;
	   viewer.getControl().setLayoutData(gd);
	   
	   fillParameterList();
	   
		return dialogArea;
	}
	
	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}
	
	/**
	 * @param manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDataType()
	 */
	@Override
	public int getParameterDataType()
	{
		return selectTables ? 0 : ((AgentParameter)selection).getDataType();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDescription()
	 */
	@Override
	public String getParameterDescription()
	{
		return selectTables ? ((AgentTable)selection).getDescription() : ((AgentParameter)selection).getDescription();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterName()
	 */
	@Override
	public String getParameterName()
	{
		return selectTables ? ((AgentTable)selection).getName() : ((AgentParameter)selection).getName();
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getInstanceColumn()
	 */
	@Override
	public String getInstanceColumn()
	{
		return selectTables ? ((AgentTable)selection).getInstanceColumn() : ""; //$NON-NLS-1$
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.isEmpty())
		{
			MessageDialogHelper.openWarning(getShell(), Messages.AbstractSelectParamDlg_Warning, Messages.AbstractSelectParamDlg_WarningText);
			return;
		}
		this.selection = selection.getFirstElement();
		saveSettings();
		super.okPressed();
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

	/**
	 * Save dialog settings
	 */
	protected void saveSettings()
	{
		Point size = getShell().getSize();
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		settings.put(getConfigurationPrefix() + ".cx", size.x); //$NON-NLS-1$
		settings.put(getConfigurationPrefix() + ".cy", size.y); //$NON-NLS-1$
	}

	/**
	 * Fill list with parameters
	 */
	protected abstract void fillParameterList();
	
	/**
	 * Get configuration prefix for saving/restoring configuration
	 * 
	 * @return configuration prefix
	 */
	protected abstract String getConfigurationPrefix();
}
