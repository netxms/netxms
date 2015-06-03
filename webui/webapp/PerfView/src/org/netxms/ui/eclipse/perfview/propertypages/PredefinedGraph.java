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
package org.netxms.ui.eclipse.perfview.propertypages;

import java.util.HashMap;
import java.util.Iterator;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.PredefinedChartConfig;
import org.netxms.ui.eclipse.perfview.propertypages.helpers.AccessListComparator;
import org.netxms.ui.eclipse.perfview.propertypages.helpers.AccessListLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.dialogs.SelectUserDialog;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Predefined graph properties property page
 */
public class PredefinedGraph extends PropertyPage
{
	private PredefinedChartConfig config;
	private LabeledText name;
	private SortableTableViewer userList;
	private HashMap<Integer, Button> accessChecks = new HashMap<Integer, Button>(2);
	private HashMap<Long, AccessListElement> acl;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (PredefinedChartConfig)getElement().getAdapter(PredefinedChartConfig.class);
		
		acl = new HashMap<Long, AccessListElement>(config.getAccessList().size());
		for(AccessListElement e : config.getAccessList())
			acl.put(e.getUserId(), new AccessListElement(e));
		
		// Initiate loading of user manager plugin if it was not loaded before
		Platform.getAdapterManager().loadAdapter(new AccessListElement(0, 0), "org.eclipse.ui.model.IWorkbenchAdapter"); //$NON-NLS-1$
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
      name = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER);
      name.setLabel(Messages.get().PredefinedGraph_Name);
      name.setText(config.getName());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      name.setLayoutData(gd);
      
		Group users = new Group(dialogArea, SWT.NONE);
		users.setText(Messages.get().PredefinedGraph_UsersAndGroups);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      users.setLayoutData(gd);

		layout = new GridLayout();
		users.setLayout(layout);
      
      final String[] columnNames = { Messages.get().PredefinedGraph_LoginName, Messages.get().PredefinedGraph_Rights };
      final int[] columnWidths = { 150, 100 };
      userList = new SortableTableViewer(users, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      userList.setContentProvider(new ArrayContentProvider());
      userList.setLabelProvider(new AccessListLabelProvider());
      userList.setComparator(new AccessListComparator());
      userList.setInput(acl.values().toArray());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      userList.getControl().setLayoutData(gd);
      
      Composite buttons = new Composite(users, SWT.NONE);
      FillLayout buttonsLayout = new FillLayout();
      buttonsLayout.spacing = WidgetHelper.INNER_SPACING;
      buttons.setLayout(buttonsLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.widthHint = 184;
      buttons.setLayoutData(gd);
      
      final Button addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(Messages.get().PredefinedGraph_Add);
      addButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				SelectUserDialog dlg = new SelectUserDialog(PredefinedGraph.this.getShell(), AbstractUserObject.class);
				if (dlg.open() == Window.OK)
				{
					AbstractUserObject[] selection = dlg.getSelection();
					for(AbstractUserObject user : selection)
						acl.put(user.getId(), new AccessListElement(user.getId(), 0));
					userList.setInput(acl.values().toArray());
				}
			}
      });

      final Button deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(Messages.get().PredefinedGraph_Delete);
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@SuppressWarnings("unchecked")
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				IStructuredSelection sel = (IStructuredSelection)userList.getSelection();
				Iterator<AccessListElement> it = sel.iterator();
				while(it.hasNext())
				{
					AccessListElement element = it.next();
					acl.remove(element.getUserId());
				}
				userList.setInput(acl.values().toArray());
			}
      });
      
      Group rights = new Group(dialogArea, SWT.NONE);
      rights.setText(Messages.get().PredefinedGraph_AccessRights);
      rights.setLayout(new RowLayout(SWT.VERTICAL));
      gd = new GridData();
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      rights.setLayoutData(gd);
      
      createAccessCheck(rights, Messages.get().PredefinedGraph_Read, GraphSettings.ACCESS_READ);
      createAccessCheck(rights, Messages.get().PredefinedGraph_Modify, GraphSettings.ACCESS_WRITE);
      
      userList.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection sel = (IStructuredSelection)event.getSelection();
				if (sel.size() == 1)
				{
					enableAllChecks(true);
					AccessListElement element = (AccessListElement)sel.getFirstElement();
					int rights = element.getAccessRights();
					for(int i = 0, mask = 1; i < 16; i++, mask <<= 1)
					{
						Button check = accessChecks.get(mask);
						if (check != null)
						{
							check.setSelection((rights & mask) == mask);
						}
					}
				}
				else
				{
					enableAllChecks(false);
				}
				deleteButton.setEnabled(sel.size() > 0);
			}
      });
      
		return dialogArea;
	}
	
	/**
	 * Create access control check box.
	 * 
	 * @param parent Parent composite
	 * @param name Name of the access right
	 * @param bitMask Bit mask for access right
	 */
	private void createAccessCheck(final Composite parent, final String name, final Integer bitMask)
	{
      final Button check = new Button(parent, SWT.CHECK);
      check.setText(name);
      check.setEnabled(false);
      check.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				IStructuredSelection sel = (IStructuredSelection)userList.getSelection();
				AccessListElement element = (AccessListElement)sel.getFirstElement();
				int rights = element.getAccessRights();
				if (check.getSelection())
					rights |= bitMask;
				else
					rights &= ~bitMask;
				element.setAccessRights(rights);
				userList.update(element, null);
			}
      });
      accessChecks.put(bitMask, check);
	}

	/**
	 * Enables all access check boxes if the argument is true, and disables them otherwise.
	 * 
	 * @param enabled the new enabled state
	 */
	private void enableAllChecks(boolean enabled)
	{
		for(final Button b : accessChecks.values())
		{
			b.setEnabled(enabled);
		}
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		config.setName(name.getText());
		config.getAccessList().clear();
		config.getAccessList().addAll(acl.values());
		if (isApply)
		{
			setValid(false);
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			new ConsoleJob(Messages.get().PredefinedGraph_JobName, null, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.saveGraph(((PredefinedChartConfig)config).createServerSettings(), true);
				}
	
				@Override
				protected void jobFinalize()
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							PredefinedGraph.this.setValid(true);
						}
					});
				}
	
				@Override
				protected String getErrorMessage()
				{
					return Messages.get().PredefinedGraph_JobError;
				}
			}.start();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		acl.clear();
		userList.setInput(acl.values().toArray());
	}
}
