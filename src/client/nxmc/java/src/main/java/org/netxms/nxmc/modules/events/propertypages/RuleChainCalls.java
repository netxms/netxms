/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.LabelProvider;
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
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.events.EventProcessingPolicyChain;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Chain Calls" property page for EPP rule - ordered list of chains entered when the rule matches
 */
public class RuleChainCalls extends RuleBasePropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(RuleChainCalls.class);

   private TableViewer viewer;
   private List<Integer> chainCalls;
   private Button addButton;
   private Button deleteButton;
   private Button upButton;
   private Button downButton;

   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleChainCalls(RuleEditor editor)
   {
      super(editor, LocalizationHelper.getI18n(RuleChainCalls.class).tr("Chain Calls"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      chainCalls = new ArrayList<>(rule.getChainCalls());

      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            EventProcessingPolicyChain chain = editor.getEditorView().findChain((Integer)element);
            return (chain != null) ? chain.getName() : String.format(i18n.tr("[%d] (unresolved)"), (Integer)element);
         }
      });
      viewer.setInput(chainCalls.toArray());
      viewer.addSelectionChangedListener((event) -> {
         int size = viewer.getStructuredSelection().size();
         deleteButton.setEnabled(size > 0);
         upButton.setEnabled(size == 1);
         downButton.setEnabled(size == 1);
      });

      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.heightHint = 0;
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

      upButton = new Button(buttons, SWT.PUSH);
      upButton.setText(i18n.tr("&Up"));
      upButton.setEnabled(false);
      upButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveSelection(-1);
         }
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      upButton.setLayoutData(rd);

      downButton = new Button(buttons, SWT.PUSH);
      downButton.setText(i18n.tr("Dow&n"));
      downButton.setEnabled(false);
      downButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveSelection(1);
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      downButton.setLayoutData(rd);

      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(i18n.tr("&Add..."));
      addButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addChainCall();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);

      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            for(Object o : viewer.getStructuredSelection().toList())
               chainCalls.remove(o);
            viewer.setInput(chainCalls.toArray());
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);

      return dialogArea;
   }

   /**
    * Move selected chain call up or down
    *
    * @param direction -1 to move up, 1 to move down
    */
   private void moveSelection(int direction)
   {
      Integer calledChainId = (Integer)viewer.getStructuredSelection().getFirstElement();
      int index = chainCalls.indexOf(calledChainId);
      int newIndex = index + direction;
      if ((index == -1) || (newIndex < 0) || (newIndex >= chainCalls.size()))
         return;
      chainCalls.set(index, chainCalls.get(newIndex));
      chainCalls.set(newIndex, calledChainId);
      viewer.setInput(chainCalls.toArray());
      viewer.setSelection(viewer.getSelection());
   }

   /**
    * Add chain call selected from available chains
    */
   private void addChainCall()
   {
      List<EventProcessingPolicyChain> availableChains = new ArrayList<>();
      for(EventProcessingPolicyChain chain : editor.getEditorView().getChains())
      {
         if (!chain.isMain() && (chain.getId() != rule.getChainId()) && !chainCalls.contains(chain.getId()))
            availableChains.add(chain);
      }
      availableChains.sort((c1, c2) -> c1.getName().compareToIgnoreCase(c2.getName()));

      ChainSelectionDialog dlg = new ChainSelectionDialog(getShell(), availableChains);
      if (dlg.open() == Window.OK)
      {
         for(EventProcessingPolicyChain chain : dlg.getSelection())
            chainCalls.add(chain.getId());
         viewer.setInput(chainCalls.toArray());
      }
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      rule.setChainCalls(new ArrayList<Integer>(chainCalls));
      editor.setModified(true);
      return true;
   }

   /**
    * Dialog for selecting chains to call
    */
   private class ChainSelectionDialog extends Dialog
   {
      private List<EventProcessingPolicyChain> chains;
      private List<EventProcessingPolicyChain> selection = new ArrayList<>();
      private TableViewer chainViewer;

      ChainSelectionDialog(Shell parentShell, List<EventProcessingPolicyChain> chains)
      {
         super(parentShell);
         this.chains = chains;
      }

      /**
       * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
       */
      @Override
      protected void configureShell(Shell newShell)
      {
         super.configureShell(newShell);
         newShell.setText(i18n.tr("Select Chain"));
      }

      /**
       * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
       */
      @Override
      protected Control createDialogArea(Composite parent)
      {
         Composite dialogArea = (Composite)super.createDialogArea(parent);
         dialogArea.setLayout(new GridLayout());

         chainViewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
         chainViewer.setContentProvider(new ArrayContentProvider());
         chainViewer.setLabelProvider(new LabelProvider() {
            @Override
            public String getText(Object element)
            {
               return ((EventProcessingPolicyChain)element).getName();
            }
         });
         chainViewer.setInput(chains.toArray());
         chainViewer.addDoubleClickListener((event) -> okPressed());

         GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
         gd.widthHint = 350;
         gd.heightHint = 300;
         chainViewer.getControl().setLayoutData(gd);

         return dialogArea;
      }

      /**
       * @see org.eclipse.jface.dialogs.Dialog#okPressed()
       */
      @Override
      protected void okPressed()
      {
         for(Object o : chainViewer.getStructuredSelection().toList())
            selection.add((EventProcessingPolicyChain)o);
         super.okPressed();
      }

      List<EventProcessingPolicyChain> getSelection()
      {
         return selection;
      }
   }
}
