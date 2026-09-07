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
package org.netxms.nxmc.modules.events.dialogs;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicyChain;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.propertypages.helpers.AccessListComparator;
import org.netxms.nxmc.modules.datacollection.propertypages.helpers.AccessListLabelProvider;
import org.netxms.nxmc.modules.users.dialogs.UserSelectionDialog;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for creating or editing event processing policy chain (name, description, access control list)
 */
public class EppChainPropertiesDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EppChainPropertiesDialog.class);

   private EventProcessingPolicyChain chain;
   private LabeledText nameText;
   private LabeledText descriptionText;
   private SortableTableViewer userList;
   private Map<Integer, Button> accessChecks = new HashMap<>(2);
   private Map<Integer, AccessListElement> acl = new HashMap<>();
   private String name;
   private String description;
   private List<AccessListElement> accessList;

   /**
    * Create dialog.
    *
    * @param parentShell parent shell
    * @param chain chain to edit or null to create a new chain
    */
   public EppChainPropertiesDialog(Shell parentShell, EventProcessingPolicyChain chain)
   {
      super(parentShell);
      this.chain = chain;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((chain != null) ? i18n.tr("Chain Properties") : i18n.tr("Create Chain"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      nameText = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER);
      nameText.setLabel(i18n.tr("Name"));
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1);
      gd.widthHint = 500;
      nameText.setLayoutData(gd);

      descriptionText = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER);
      descriptionText.setLabel(i18n.tr("Description"));
      descriptionText.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      if (chain != null)
      {
         nameText.setText(chain.getName());
         descriptionText.setText((chain.getDescription() != null) ? chain.getDescription() : "");
         for(AccessListElement e : chain.getAccessList())
            acl.put(e.getUserId(), new AccessListElement(e));
      }

      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Synchronizing missing users"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if (session.syncMissingUsers(acl.keySet()))
               runInUIThread(() -> userList.refresh(true));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot synchronize users");
         }
      };
      job.setUser(false);
      job.start();

      Group users = new Group(dialogArea, SWT.NONE);
      users.setText(i18n.tr("Users and Groups"));
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.heightHint = 250;
      users.setLayoutData(gd);
      users.setLayout(new GridLayout());

      final String[] columnNames = { i18n.tr("Login name"), i18n.tr("Rights") };
      final int[] columnWidths = { 250, 100 };
      userList = new SortableTableViewer(users, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      userList.setContentProvider(new ArrayContentProvider());
      userList.setLabelProvider(new AccessListLabelProvider());
      userList.setComparator(new AccessListComparator());
      userList.setInput(acl.values().toArray());
      userList.getControl().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      Composite buttons = new Composite(users, SWT.NONE);
      FillLayout buttonsLayout = new FillLayout();
      buttonsLayout.spacing = WidgetHelper.INNER_SPACING;
      buttons.setLayout(buttonsLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.widthHint = 184;
      buttons.setLayoutData(gd);

      final Button addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(i18n.tr("&Add..."));
      addButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            UserSelectionDialog dlg = new UserSelectionDialog(getShell(), AbstractUserObject.class);
            if (dlg.open() == Window.OK)
            {
               for(AbstractUserObject user : dlg.getSelection())
                  acl.put(user.getId(), new AccessListElement(user.getId(), EventProcessingPolicyChain.ACCESS_READ));
               userList.setInput(acl.values().toArray());
            }
         }
      });

      final Button deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionAdapter() {
         @SuppressWarnings("unchecked")
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            Iterator<AccessListElement> it = userList.getStructuredSelection().iterator();
            while(it.hasNext())
               acl.remove(it.next().getUserId());
            userList.setInput(acl.values().toArray());
         }
      });

      Group rights = new Group(dialogArea, SWT.NONE);
      rights.setText(i18n.tr("Access Rights"));
      rights.setLayout(new RowLayout(SWT.VERTICAL));
      rights.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      createAccessCheck(rights, i18n.tr("&Read"), EventProcessingPolicyChain.ACCESS_READ);
      createAccessCheck(rights, i18n.tr("&Edit"), EventProcessingPolicyChain.ACCESS_EDIT);

      userList.addSelectionChangedListener((event) -> {
         IStructuredSelection sel = event.getStructuredSelection();
         if (sel.size() == 1)
         {
            AccessListElement element = (AccessListElement)sel.getFirstElement();
            for(Map.Entry<Integer, Button> entry : accessChecks.entrySet())
            {
               entry.getValue().setEnabled(true);
               entry.getValue().setSelection((element.getAccessRights() & entry.getKey()) != 0);
            }
         }
         else
         {
            for(Button check : accessChecks.values())
            {
               check.setEnabled(false);
               check.setSelection(false);
            }
         }
         deleteButton.setEnabled(sel.size() > 0);
      });

      return dialogArea;
   }

   /**
    * Create access control check box.
    *
    * @param parent parent composite
    * @param name name of the access right
    * @param bitMask bit mask for access right
    */
   private void createAccessCheck(Composite parent, String name, final Integer bitMask)
   {
      final Button check = new Button(parent, SWT.CHECK);
      check.setText(name);
      check.setEnabled(false);
      check.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            AccessListElement element = (AccessListElement)userList.getStructuredSelection().getFirstElement();
            int rights = (int)element.getAccessRights();
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
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      name = nameText.getText().trim();
      if (name.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Chain name cannot be empty"));
         return;
      }
      description = descriptionText.getText().trim();
      accessList = new ArrayList<>(acl.values());
      super.okPressed();
   }

   /**
    * Get chain name entered by user.
    *
    * @return chain name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get chain description entered by user.
    *
    * @return chain description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * Get access control list configured by user.
    *
    * @return access control list
    */
   public List<AccessListElement> getAccessList()
   {
      return accessList;
   }
}
