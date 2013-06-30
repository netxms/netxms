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
package org.netxms.ui.eclipse.epp.propertypages;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.epp.dialogs.AttributeEditDialog;
import org.netxms.ui.eclipse.epp.propertypages.helpers.AttributeLabelProvider;
import org.netxms.ui.eclipse.epp.widgets.RuleEditor;
import org.netxms.ui.eclipse.epp.widgets.SituationSelector;
import org.netxms.ui.eclipse.tools.ObjectLabelComparator;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "Events" property page for EPP rule
 *
 */
public class RuleSituation extends PropertyPage
{
	private RuleEditor editor;
	private EventProcessingPolicyRule rule;
	private SituationSelector situation;
	private LabeledText instance;
	private SortableTableViewer viewer;
	private Map<String, String> attributes = new HashMap<String, String>();
	private Button addButton;
	private Button editButton;
	private Button deleteButton;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		editor = (RuleEditor)getElement().getAdapter(RuleEditor.class);
		rule = editor.getRule();
		attributes.putAll(rule.getSituationAttributes());
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      final int vInd = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;
      
      situation = new SituationSelector(dialogArea, SWT.NONE);
      situation.setLabel("Situation object");
      situation.setSituationId(rule.getSituationId());
      GridData gd = new GridData();
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      situation.setLayoutData(gd);
      
      instance = new LabeledText(dialogArea, SWT.NONE);
      instance.setLabel("Instance");
      instance.setText(rule.getSituationInstance());
      gd = new GridData();
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalIndent = vInd;
      instance.setLayoutData(gd);
      
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Attributes");
      gd = new GridData();
      gd.verticalIndent = vInd;
      label.setLayoutData(gd);
      
      final String[] columnNames = { "Name", "Value" };
      final int[] columnWidths = { 150, 200 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new AttributeLabelProvider());
      viewer.setComparator(new ObjectLabelComparator((ILabelProvider)viewer.getLabelProvider()));
      viewer.setInput(attributes.entrySet().toArray());
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				int size = ((IStructuredSelection)viewer.getSelection()).size();
				editButton.setEnabled(size == 1);
				deleteButton.setEnabled(size > 0);
			}
		});

      gd = new GridData();
      gd.verticalAlignment = GridData.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      viewer.getControl().setLayoutData(gd);
      
      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gd);

      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText("&Add...");
      addButton.addSelectionListener(new SelectionListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addAttribute();
			}
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);
		
      editButton = new Button(buttons, SWT.PUSH);
      editButton.setText("&Edit...");
      editButton.addSelectionListener(new SelectionListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				editAttribute();
			}
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(rd);
      editButton.setEnabled(false);
		
      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText("&Delete");
      deleteButton.addSelectionListener(new SelectionListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				deleteAttribute();
			}
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
      deleteButton.setEnabled(false);
		
		return dialogArea;
	}

	/**
	 * Add new attribute
	 */
	private void addAttribute()
	{
		AttributeEditDialog dlg = new AttributeEditDialog(getShell(), null, null);
		if (dlg.open() == Window.OK)
		{
			attributes.put(dlg.getAtributeName(), dlg.getAttributeValue());
	      viewer.setInput(attributes.entrySet().toArray());
		}
	}
	
	/**
	 * Edit selected attribute
	 */
	@SuppressWarnings("unchecked")
	private void editAttribute()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		
		Entry<String, String> attr = (Entry<String, String>)selection.getFirstElement();
		AttributeEditDialog dlg = new AttributeEditDialog(getShell(), attr.getKey(), attr.getValue());
		if (dlg.open() == Window.OK)
		{
			attributes.put(dlg.getAtributeName(), dlg.getAttributeValue());
	      viewer.setInput(attributes.entrySet().toArray());
		}
	}
	
	/**
	 * Delete attribute(s) from list
	 */
	@SuppressWarnings("unchecked")
	private void deleteAttribute()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		Iterator<Entry<String, String>> it = selection.iterator();
		if (it.hasNext())
		{
			while(it.hasNext())
			{
				Entry<String, String> e = it.next();
				attributes.remove(e.getKey());
			}
	      viewer.setInput(attributes.entrySet().toArray());
		}
	}
	
	/**
	 * Update rule object
	 */
	private void doApply()
	{
		rule.setSituationId(situation.getSituationId());
		rule.setSituationInstance(instance.getText());
		rule.setSituationAttributes(attributes);
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
