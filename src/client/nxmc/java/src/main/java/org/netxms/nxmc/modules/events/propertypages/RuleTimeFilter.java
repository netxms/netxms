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
package org.netxms.nxmc.modules.events.propertypages;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.eclipse.jface.viewers.ArrayContentProvider;
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
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.TimeFrame;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.dialogs.TimeFrameEditorDialog;
import org.netxms.nxmc.modules.events.propertypages.helpers.TimeFrameLabelProvider;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;
import org.netxms.nxmc.tools.ElementLabelComparator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Time Filter" property page for EPP rule
 */
public class RuleTimeFilter extends RuleBasePropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(RuleTimeFilter.class);
   private final String EMPTY_LIST_PLACEHOLDER[] = { i18n.tr("Any time") };

   private List<TimeFrame> frames = new ArrayList<TimeFrame>();
   private TableViewer viewer;
   private Button addButton;
   private Button editButton;
   private Button deleteButton;
   private Button checkInverted;

   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleTimeFilter(RuleEditor editor)
   {
      super(editor, LocalizationHelper.getI18n(RuleTimeFilter.class).tr("Time Filter"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 1;
      dialogArea.setLayout(layout);

      checkInverted = new Button(dialogArea, SWT.CHECK);
      checkInverted.setText(i18n.tr("Inverse rule (match time that is NOT within time frames listed below)"));
      checkInverted.setSelection(rule.isTimeFramesInverted());

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new TimeFrameLabelProvider());
      viewer.setComparator(new ElementLabelComparator((ILabelProvider)viewer.getLabelProvider()));
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
            IStructuredSelection selection = viewer.getStructuredSelection();
            int size = selection.size();
            boolean placeholder = selection.getFirstElement() instanceof String;
            deleteButton.setEnabled(!placeholder && (size > 0));
            editButton.setEnabled(!placeholder && (size == 1));
			}
      });

      frames.addAll(rule.getTimeFrames());
      setViewerInput();

      GridData gd = new GridData();
      gd.verticalAlignment = GridData.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 0;
      gd.horizontalSpan = 1;
      viewer.getControl().setLayoutData(gd);      
      viewer.addDoubleClickListener((event) -> editFrame());

      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gd);

      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(i18n.tr("&Add..."));
      addButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addFrame();
         }
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);

      editButton = new Button(buttons, SWT.PUSH);
      editButton.setText(i18n.tr("&Edit..."));
      editButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editFrame();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(rd);
      
      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
      deleteButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteFrame();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);

		return dialogArea;
	}

	/**
    * Add new time frame
    */
	private void addFrame()
	{
	   TimeFrameEditorDialog dlg = new TimeFrameEditorDialog(getShell(), null);
		if (dlg.open() == Window.OK)
		{
		   frames.add(dlg.getTimeFrame());
         setViewerInput();
		}
	}

   /**
    * Delete event from list
    */
   private void editFrame()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if ((selection.size() == 1) && !(selection.getFirstElement() instanceof String))
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
      IStructuredSelection selection = viewer.getStructuredSelection();
		Iterator<?> it = selection.iterator();
		if (it.hasNext())
		{
			while(it.hasNext())
			{
			   TimeFrame e = (TimeFrame)it.next();
			   frames.remove(e);
			}
         setViewerInput();
		}
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      int flags = rule.getFlags();
      if (checkInverted.getSelection() && !frames.isEmpty()) // ignore "negate" flag if frames set is empty
         flags |= EventProcessingPolicyRule.NEGATED_TIME_FRAMES;
      else
         flags &= ~EventProcessingPolicyRule.NEGATED_TIME_FRAMES;
      rule.setFlags(flags);
      rule.setTimeFrames(frames);
		editor.setModified(true);
		return true;
	}

   /**
    * Set input for source object viewer
    */
   private void setViewerInput()
   {
      if (frames.isEmpty())
         viewer.setInput(EMPTY_LIST_PLACEHOLDER);
      else
         viewer.setInput(frames);
   }
}
