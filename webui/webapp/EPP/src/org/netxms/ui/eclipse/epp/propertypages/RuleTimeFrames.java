/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.TimeFrame;
import org.netxms.ui.eclipse.epp.dialogs.TimeFrameEditorDialog;
import org.netxms.ui.eclipse.epp.propertypages.helpers.TimeFrameLabelProvider;
import org.netxms.ui.eclipse.epp.widgets.RuleEditor;
import org.netxms.ui.eclipse.tools.ElementLabelComparator;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Time" property page for EPP rule
 */
public class RuleTimeFrames extends PropertyPage
{
	private RuleEditor editor;
	private EventProcessingPolicyRule rule;
   private List<TimeFrame> frames = new ArrayList<TimeFrame>();
	private TableViewer viewer;
	private Button addButton;
   private Button editButton;
	private Button deleteButton;
	private Button checkInverted;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		editor = (RuleEditor)getElement().getAdapter(RuleEditor.class);
		rule = editor.getRule();
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 1;
      dialogArea.setLayout(layout);
      
      checkInverted = new Button(dialogArea, SWT.CHECK);
      checkInverted.setText("Invert rule (match time frames NOT in the list below)");
      checkInverted.setSelection(rule.isTimeFramesInverted());
      
      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new TimeFrameLabelProvider());
      viewer.setComparator(new ElementLabelComparator((ILabelProvider)viewer.getLabelProvider()));
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				int size = ((IStructuredSelection)viewer.getSelection()).size();
				deleteButton.setEnabled(size > 0);
            editButton.setEnabled(size == 1);
			}
      });

      frames.addAll(rule.getTimeFrames());
      viewer.setInput(frames);

      GridData gd = new GridData();
      gd.verticalAlignment = GridData.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 0;
      gd.horizontalSpan = 1;
      viewer.getControl().setLayoutData(gd);      
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editFrame();
         }
      });

      Composite rightButtons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      rightButtons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      rightButtons.setLayoutData(gd);

      addButton = new Button(rightButtons, SWT.PUSH);
      addButton.setText("Add");
      addButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addFrame();
         }
      });

      editButton = new Button(rightButtons, SWT.PUSH);
      editButton.setText("Edit");
      editButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editFrame();
         }
      });
      
      deleteButton = new Button(rightButtons, SWT.PUSH);
      deleteButton.setText("Delete");
      deleteButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteFrame();
         }
      });
		
		return dialogArea;
	}

	/**
	 * Add new event
	 */
	private void addFrame()
	{
	   TimeFrameEditorDialog dlg = new TimeFrameEditorDialog(getShell(), null);
		if (dlg.open() == Window.OK)
		{
		   frames.add(dlg.getTimeFrame());
	      viewer.setInput(frames);
		}
	}
   
   /**
    * Delete event from list
    */
   private void editFrame()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() == 1)
      {
         TimeFrame element = (TimeFrame)selection.getFirstElement();

         TimeFrameEditorDialog dlg = new TimeFrameEditorDialog(getShell(), element);
         if (dlg.open() == Window.OK)
         {
            viewer.update(element, null);   
         }
      }
   }
	
	/**
	 * Delete event from list
	 */
	private void deleteFrame()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		Iterator<?> it = selection.iterator();
		if (it.hasNext())
		{
			while(it.hasNext())
			{
			   TimeFrame e = (TimeFrame)it.next();
			   frames.remove(e);
			}
	      viewer.setInput(frames);
		}
	}
	
	/**
	 * Update rule object
	 */
	private void doApply()
	{
      int flags = rule.getFlags();
      if (checkInverted.getSelection() && !frames.isEmpty()) // ignore "negate" flag if frames set is empty
         flags |= EventProcessingPolicyRule.NEGATED_TIME_FRAMES;
      else
         flags &= ~EventProcessingPolicyRule.NEGATED_TIME_FRAMES;
      rule.setFlags(flags);
      rule.setTimeFrames(frames);
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
