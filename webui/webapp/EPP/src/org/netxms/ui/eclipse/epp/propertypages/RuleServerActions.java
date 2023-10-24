/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.epp.propertypages;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.ServerAction;
import org.netxms.client.events.ActionExecutionConfiguration;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.epp.Messages;
import org.netxms.ui.eclipse.epp.dialogs.ActionExecutionConfigurationDialog;
import org.netxms.ui.eclipse.epp.dialogs.ActionSelectionDialog;
import org.netxms.ui.eclipse.epp.propertypages.helpers.ActionListLabelProvider;
import org.netxms.ui.eclipse.epp.widgets.RuleEditor;
import org.netxms.ui.eclipse.tools.ElementLabelComparator;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "Actions" property page for EPP rule
 */
public class RuleServerActions extends PropertyPage
{
	private RuleEditor editor;
	private EventProcessingPolicyRule rule;
	private SortableTableViewer viewer;
   private List<ActionExecutionConfiguration> actions = new ArrayList<ActionExecutionConfiguration>();
   private Button addButton;
	private Button editButton;
	private Button deleteButton;

	/**
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		editor = (RuleEditor)getElement().getAdapter(RuleEditor.class);
		rule = editor.getRule();

		Composite dialogArea = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      final String[] columnNames = { Messages.get().RuleServerActions_Action, "Delay", "Delay timer key", "Snooze time", "Snooze/blocking timer key" };
      final int[] columnWidths = { 300, 90, 200, 90, 200 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ActionListLabelProvider(editor.getEditorView()));
      viewer.setComparator(new ElementLabelComparator((ILabelProvider)viewer.getLabelProvider()));
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
            int size = viewer.getStructuredSelection().size();
				deleteButton.setEnabled(size > 0);
			}
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editAction();
         }
      });

      for(ActionExecutionConfiguration c : rule.getActions())
         actions.add(new ActionExecutionConfiguration(c));
      viewer.setInput(actions);

      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.heightHint = 0;
      viewer.getControl().setLayoutData(gridData);

      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gridData);

      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(Messages.get().RuleServerActions_Add);
      addButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addAction();
			}
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);

      editButton = new Button(buttons, SWT.PUSH);
      editButton.setText("&Edit...");
      editButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editAction();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(rd);
      
      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(Messages.get().RuleServerActions_Delete);
      deleteButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				deleteAction();
			}
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
      deleteButton.setEnabled(false);
		
		return dialogArea;
	}

	/**
	 * Add new event
	 */
	private void addAction()
	{
		ActionSelectionDialog dlg = new ActionSelectionDialog(getShell(), editor.getEditorView().getActions());
		dlg.enableMultiSelection(true);
		if (dlg.open() == Window.OK)
		{
			for(ServerAction a : dlg.getSelectedActions())
				actions.add(new ActionExecutionConfiguration(a.getId(), null, null, null, null));
		}
      viewer.refresh();
	}

	/**
	 * Edit current action
	 */
	private void editAction()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      ActionExecutionConfigurationDialog dlg = new ActionExecutionConfigurationDialog(getShell(), (ActionExecutionConfiguration)selection.getFirstElement());
      if (dlg.open() == Window.OK)
      {
         viewer.update(selection.getFirstElement(), null);
      }
	}

	/**
	 * Delete event from list
	 */
	private void deleteAction()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
		Iterator<?> it = selection.iterator();
		if (it.hasNext())
		{
			while(it.hasNext())
			{
			   ActionExecutionConfiguration a = (ActionExecutionConfiguration)it.next();
            actions.remove(a);
			}
         viewer.refresh();
		}
	}

	/**
	 * Update rule object
	 */
	private void doApply()
	{
      rule.setActions(actions);
		editor.setModified(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		doApply();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		doApply();
		return super.performOk();
	}
}
