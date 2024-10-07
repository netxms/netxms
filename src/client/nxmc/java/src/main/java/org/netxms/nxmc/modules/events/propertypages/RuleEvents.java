/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.dialogs.EventSelectionDialog;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;
import org.netxms.nxmc.modules.events.widgets.helpers.BaseEventTemplateLabelProvider;
import org.netxms.nxmc.tools.ElementLabelComparator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Events" property page for EPP rule
 */
public class RuleEvents extends RuleBasePropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(RuleEvents.class);
   private final String EMPTY_LIST_PLACEHOLDER[] = { i18n.tr("Any event") };

	private NXCSession session;
	private SortableTableViewer viewer;
   private Map<Integer, EventTemplate> events = new HashMap<>();
	private Button addButton;
	private Button deleteButton;
	private Button checkInverted;

   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleEvents(RuleEditor editor)
   {
      super(editor, LocalizationHelper.getI18n(RuleEvents.class).tr("Events"));
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
      checkInverted.setText(i18n.tr("Inverse rule (match events NOT listed below)"));
      checkInverted.setSelection(rule.isEventsInverted());

      final String[] columnNames = { i18n.tr("Events") };
      final int[] columnWidths = { 300 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new EventListLabelProvider());
      viewer.setComparator(new ElementLabelComparator((ILabelProvider)viewer.getLabelProvider()));
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
            IStructuredSelection selection = viewer.getStructuredSelection();
            int size = selection.size();
            deleteButton.setEnabled((size > 0) && !(selection.getFirstElement() instanceof String));
			}
      });

      for(EventTemplate o : session.findMultipleEventTemplates(rule.getEvents()))
      	events.put(o.getCode(), o);
      setViewerInput();

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
				addEvent();
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
				deleteEvent();
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
	private void addEvent()
	{
		EventSelectionDialog dlg = new EventSelectionDialog(getShell());
		dlg.enableMultiSelection(true);
		if (dlg.open() == Window.OK)
		{
			for(EventTemplate e : dlg.getSelectedEvents())
				events.put(e.getCode(), e);
		}
      setViewerInput();
	}

	/**
	 * Delete event from list
	 */
	private void deleteEvent()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
		Iterator<?> it = selection.iterator();
		if (it.hasNext())
		{
			while(it.hasNext())
			{
			   EventTemplate e = (EventTemplate)it.next();
				events.remove(e.getCode());
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
      if (checkInverted.getSelection() && !events.isEmpty()) // ignore "negate" flag if event set is empty
         flags |= EventProcessingPolicyRule.NEGATED_EVENTS;
      else
         flags &= ~EventProcessingPolicyRule.NEGATED_EVENTS;
      rule.setFlags(flags);
      rule.setEvents(new ArrayList<Integer>(events.keySet()));
		editor.setModified(true);
      return true;
	}

   /**
    * Set input for source object viewer
    */
   private void setViewerInput()
   {
      if (events.isEmpty())
         viewer.setInput(EMPTY_LIST_PLACEHOLDER);
      else
         viewer.setInput(events.values().toArray());
   }

   /**
    * Label provider for event list
    */
   private static class EventListLabelProvider extends BaseEventTemplateLabelProvider implements IColorProvider
   {
      private static final Color HINT_FOREGROUND = new Color(Display.getDefault(), 192, 192, 192);

      /**
       * @see org.netxms.nxmc.modules.events.widgets.helpers.BaseEventTemplateLabelProvider#getImage(java.lang.Object)
       */
      @Override
      public Image getImage(Object element)
      {
         return (element instanceof String) ? null : super.getImage(element);
      }

      /**
       * @see org.netxms.nxmc.modules.events.widgets.helpers.BaseEventTemplateLabelProvider#getText(java.lang.Object)
       */
      @Override
      public String getText(Object element)
      {
         return (element instanceof String) ? (String)element : super.getText(element);
      }

      /**
       * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
       */
      @Override
      public Color getForeground(Object element)
      {
         if (element instanceof String)
            return HINT_FOREGROUND;
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
       */
      @Override
      public Color getBackground(Object element)
      {
         return null;
      }
   }
}
