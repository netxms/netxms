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
package org.netxms.nxmc.modules.datacollection.dialogs;

import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.Dialog;
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
import org.netxms.client.constants.DataType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.helpers.AgentParameterComparator;
import org.netxms.nxmc.modules.datacollection.dialogs.helpers.AgentParameterFilter;
import org.netxms.nxmc.modules.datacollection.dialogs.helpers.AgentParameterLabelProvider;
import org.netxms.nxmc.modules.datacollection.dialogs.helpers.AgentTableComparator;
import org.netxms.nxmc.modules.datacollection.dialogs.helpers.AgentTableLabelProvider;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Abstract base class for metric selection dialogs.
 */
public abstract class AbstractSelectParamDlg extends Dialog implements IParameterSelectionDialog
{
   private final I18n i18n = LocalizationHelper.getI18n(AbstractSelectParamDlg.class);
   
   public static final int COLUMN_DESCRIPTION = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_TYPE = 2;

	
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
		object = Registry.getSession().findObjectById(nodeId);
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(selectTables ? i18n.tr("Table Selection") : i18n.tr("Parameter Selection"));
      PreferenceStore settings = PreferenceStore.getInstance();
		final String prefix = getConfigurationPrefix();
		try
		{
			newShell.setSize(settings.getAsPoint(prefix + ".size", 600, 400)); //$NON-NLS-1$ //$NON-NLS-2$
		}
		catch(NumberFormatException e)
		{
		}
	}

   /**
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
	   label.setText(selectTables ? i18n.tr("Available tables") : i18n.tr("Available parameters"));
	   
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
		
		final String[] names = { i18n.tr("Description"),  i18n.tr("Name"), selectTables ? i18n.tr("Instance Column"):i18n.tr("Data Type") };
		final int[] widths = { 350, 150, selectTables ? 150 : 100 };
	   viewer = new SortableTableViewer(dialogArea, names, widths, 0, SWT.UP, SWT.FULL_SELECTION | SWT.BORDER);
	   WidgetHelper.restoreTableViewerSettings(viewer, getConfigurationPrefix() + ".viewer"); //$NON-NLS-1$
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
            WidgetHelper.saveTableViewerSettings(viewer, getConfigurationPrefix());
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
	   
	   fillList();
	   
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

   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDataType()
    */
	@Override
	public DataType getParameterDataType()
	{
		return selectTables ? DataType.NULL : ((AgentParameter)selection).getDataType();
	}

   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDescription()
    */
	@Override
	public String getParameterDescription()
	{
		return selectTables ? ((AgentTable)selection).getDescription() : ((AgentParameter)selection).getDescription();
	}

   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterName()
    */
	@Override
	public String getParameterName()
	{
		return selectTables ? ((AgentTable)selection).getName() : ((AgentParameter)selection).getName();
	}
	
   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getInstanceColumn()
    */
	@Override
	public String getInstanceColumn()
	{
		return selectTables ? ((AgentTable)selection).getInstanceColumnsAsList() : ""; //$NON-NLS-1$
	}
	
   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		IStructuredSelection selection = viewer.getStructuredSelection();
		if (selection.isEmpty())
		{
			MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("You must select parameter from the list!"));
			return;
		}
		this.selection = selection.getFirstElement();
		saveSettings();
		super.okPressed();
	}
	
   /**
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
		PreferenceStore settings = PreferenceStore.getInstance();

		settings.set(getConfigurationPrefix() + ".size", size); //$NON-NLS-1$
	}

	/**
	 * Fill list with parameters
	 */
	protected abstract void fillList();
	
	/**
	 * Get configuration prefix for saving/restoring configuration
	 * 
	 * @return configuration prefix
	 */
	protected abstract String getConfigurationPrefix();
}
