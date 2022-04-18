/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.events.propertypages;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.tools.ElementLabelComparator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Source objects" property page for EPP rule
 */
public class RuleSourceObjects extends RuleBasePropertyPage
{
   private static final I18n i18n = LocalizationHelper.getI18n(RuleSourceObjects.class);

	private NXCSession session;
	private SortableTableViewer viewer;
	private Map<Long, AbstractObject> objects = new HashMap<Long, AbstractObject>();
	private Button addButton;
	private Button deleteButton;
	private Button checkInverted;
	
   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleSourceObjects(RuleEditor editor)
   {
      super(editor, i18n.tr("Source Objects"));
      session = Registry.getSession();
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      checkInverted = new Button(dialogArea, SWT.CHECK);
      checkInverted.setText(i18n.tr("Inverted rule (match objects NOT in the list below)"));
      checkInverted.setSelection(rule.isSourceInverted());
      
      viewer = new SortableTableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new BaseObjectLabelProvider());
      viewer.setComparator(new ElementLabelComparator((ILabelProvider)viewer.getLabelProvider()));
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				int size = ((IStructuredSelection)viewer.getSelection()).size();
				deleteButton.setEnabled(size > 0);
			}
      });

      for(AbstractObject o : session.findMultipleObjects(rule.getSources(), true))
      	objects.put(o.getObjectId(), o);
      viewer.setInput(objects.values().toArray());

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
      addButton.setText(i18n.tr("&Add..."));
      addButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addSourceObject();
			}
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);
		
      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
      deleteButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				deleteSourceObject();
			}
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);

		return dialogArea;
	}

	/**
	 * Add new source object
	 */
	private void addSourceObject()
	{
      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell());
		dlg.enableMultiSelection(true);
		if (dlg.open() == Window.OK)
		{
			for(AbstractObject o : dlg.getSelectedObjects())
				objects.put(o.getObjectId(), o);
		}
      viewer.setInput(objects.values().toArray());
	}
	
	/**
	 * Delete object from list
	 */
	@SuppressWarnings("unchecked")
	private void deleteSourceObject()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		Iterator<AbstractObject> it = selection.iterator();
		if (it.hasNext())
		{
			while(it.hasNext())
			{
				AbstractObject o = it.next();
				objects.remove(o.getObjectId());
			}
	      viewer.setInput(objects.values().toArray());
		}
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
		int flags = rule.getFlags();
		if (checkInverted.getSelection() && !objects.isEmpty()) // ignore "negate" flag if object set is empty
			flags |= EventProcessingPolicyRule.NEGATED_SOURCE;
		else
			flags &= ~EventProcessingPolicyRule.NEGATED_SOURCE;
		rule.setFlags(flags);
		rule.setSources(new ArrayList<Long>(objects.keySet()));
		editor.setModified(true);
      return true;
	}
}
