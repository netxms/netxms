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
import org.eclipse.jface.viewers.TableViewer;
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
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
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
   private final I18n i18n = LocalizationHelper.getI18n(RuleSourceObjects.class);
   private final String EMPTY_LIST_PLACEHOLDER[] = { i18n.tr("Any object") };

	private NXCSession session;
	private TableViewer sourceViewer;
	private TableViewer excludeViewer;
	private Map<Long, AbstractObject> objects = new HashMap<Long, AbstractObject>();
   private Map<Long, AbstractObject> excludedObjects = new HashMap<Long, AbstractObject>();
	private Button addButtonSource;
	private Button deleteButtonSource;
	private Button addButtonExclusion;
   private Button deleteButtonExclusion;
	private Button checkInverted;

   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleSourceObjects(RuleEditor editor)
   {
      super(editor, LocalizationHelper.getI18n(RuleSourceObjects.class).tr("Source Objects"));
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
      checkInverted.setText(i18n.tr("Inverse rule (match objects NOT listed below)"));
      checkInverted.setSelection(rule.isSourceInverted());

      /* source */
      new Label(dialogArea, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      sourceViewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      sourceViewer.setContentProvider(new ArrayContentProvider());
      sourceViewer.setLabelProvider(new SourceObjectLabelProvider());
      sourceViewer.setComparator(new ElementLabelComparator((ILabelProvider)sourceViewer.getLabelProvider()));
      sourceViewer.getTable().setSortDirection(SWT.UP);
      sourceViewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
            IStructuredSelection selection = sourceViewer.getStructuredSelection();
            int size = selection.size();
            deleteButtonSource.setEnabled((size > 0) && !(selection.getFirstElement() instanceof String));
			}
      });

      for(AbstractObject o : session.findMultipleObjects(rule.getSources(), true))
      	objects.put(o.getObjectId(), o);
      setSourceViewerInput();

      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.heightHint = 0;
      sourceViewer.getControl().setLayoutData(gridData);

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

      addButtonSource = new Button(buttons, SWT.PUSH);
      addButtonSource.setText(i18n.tr("&Add..."));
      addButtonSource.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addSourceObject();
			}
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButtonSource.setLayoutData(rd);
		
      deleteButtonSource = new Button(buttons, SWT.PUSH);
      deleteButtonSource.setText(i18n.tr("&Delete"));
      deleteButtonSource.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				deleteSourceObject();
			}
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButtonSource.setLayoutData(rd);

      /* exclusions */
      new Label(dialogArea, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Exclusions:");   
      
      excludeViewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      excludeViewer.setContentProvider(new ArrayContentProvider());
      excludeViewer.setLabelProvider(new BaseObjectLabelProvider());
      excludeViewer.setComparator(new ElementLabelComparator((ILabelProvider)excludeViewer.getLabelProvider()));
      excludeViewer.getTable().setSortDirection(SWT.UP);
      excludeViewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            int size = excludeViewer.getStructuredSelection().size();
            deleteButtonExclusion.setEnabled(size > 0);
         }
      });

      for(AbstractObject o : session.findMultipleObjects(rule.getSourceExclusions(), true))
         excludedObjects.put(o.getObjectId(), o);
      excludeViewer.setInput(excludedObjects.values().toArray());

      gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.heightHint = 0;
      excludeViewer.getControl().setLayoutData(gridData);

      buttons = new Composite(dialogArea, SWT.NONE);
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gridData);

      addButtonExclusion = new Button(buttons, SWT.PUSH);
      addButtonExclusion.setText(i18n.tr("&Add..."));
      addButtonExclusion.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addExclusionObject();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButtonExclusion.setLayoutData(rd);
      
      deleteButtonExclusion = new Button(buttons, SWT.PUSH);
      deleteButtonExclusion.setText(i18n.tr("&Delete"));
      deleteButtonExclusion.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteExclusionObject();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButtonExclusion.setLayoutData(rd);

		return dialogArea;
	}

   /**
    * Set input for source object viewer
    */
   private void setSourceViewerInput()
   {
      if (objects.isEmpty())
         sourceViewer.setInput(EMPTY_LIST_PLACEHOLDER);
      else
         sourceViewer.setInput(objects.values().toArray());
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
      setSourceViewerInput();
	}

	/**
	 * Delete object from list
	 */
	@SuppressWarnings("unchecked")
	private void deleteSourceObject()
	{
		IStructuredSelection selection = sourceViewer.getStructuredSelection();
      if ((selection.size() == 1) && (selection.getFirstElement() instanceof String))
         return;

		Iterator<AbstractObject> it = selection.iterator();
		if (it.hasNext())
		{
			while(it.hasNext())
			{
				AbstractObject o = it.next();
	         objects.remove(o.getObjectId());
			}
         setSourceViewerInput();
		}
	}

   /**
    * Add new source object
    */
   private void addExclusionObject()
   {
      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell());
      dlg.enableMultiSelection(true);
      if (dlg.open() == Window.OK)
      {
         for(AbstractObject o : dlg.getSelectedObjects())
         { 
            excludedObjects.put(o.getObjectId(), o);
         }
      }
      excludeViewer.setInput(excludedObjects.values().toArray());
   }
   
   /**
    * Delete object from list
    */
   @SuppressWarnings("unchecked")
   private void deleteExclusionObject()
   {
      IStructuredSelection selection = excludeViewer.getStructuredSelection();
         
      Iterator<AbstractObject> it = selection.iterator();
      if (it.hasNext())
      {
         while(it.hasNext())
         {
            AbstractObject o = it.next();
            excludedObjects.remove(o.getObjectId());
         }
         excludeViewer.setInput(excludedObjects.values().toArray());
      }
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
		int flags = rule.getFlags();
		if (checkInverted.getSelection() && (!objects.isEmpty() || !excludedObjects.isEmpty())) // ignore "negate" flag if object set is empty
			flags |= EventProcessingPolicyRule.NEGATED_SOURCE;
		else
			flags &= ~EventProcessingPolicyRule.NEGATED_SOURCE;
		rule.setFlags(flags);
		rule.setSources(new ArrayList<Long>(objects.keySet()));
      rule.setSourceExclusions(new ArrayList<Long>(excludedObjects.keySet()));
		editor.setModified(true);
      return true;
	}

   /**
    * Label provider for source object list
    */
   private static class SourceObjectLabelProvider extends BaseObjectLabelProvider implements IColorProvider
   {
      private static final Color HINT_FOREGROUND = new Color(Display.getDefault(), 192, 192, 192);

      /**
       * @see org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider#getImage(java.lang.Object)
       */
      @Override
      public Image getImage(Object element)
      {
         return (element instanceof String) ? null : super.getImage(element);
      }

      /**
       * @see org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider#getText(java.lang.Object)
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
