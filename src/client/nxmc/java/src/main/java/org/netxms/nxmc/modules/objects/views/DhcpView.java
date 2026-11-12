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
package org.netxms.nxmc.modules.objects.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCException;
import org.netxms.client.constants.RCC;
import org.netxms.client.dhcp.DhcpLease;
import org.netxms.client.dhcp.DhcpScope;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.actions.CopyTableRowsAction;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.DhcpLeaseComparator;
import org.netxms.nxmc.modules.objects.views.helpers.DhcpLeaseFilter;
import org.netxms.nxmc.modules.objects.views.helpers.DhcpLeaseLabelProvider;
import org.netxms.nxmc.modules.objects.views.helpers.DhcpScopeComparator;
import org.netxms.nxmc.modules.objects.views.helpers.DhcpScopeLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * DHCP server view - scopes and leases
 */
public class DhcpView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(DhcpView.class);

   public static final int SCOPE_NETWORK = 0;
   public static final int SCOPE_NAME = 1;
   public static final int SCOPE_RANGES = 2;
   public static final int SCOPE_TOTAL = 3;
   public static final int SCOPE_USED = 4;
   public static final int SCOPE_DECLINED = 5;
   public static final int SCOPE_FREE = 6;
   public static final int SCOPE_UTILIZATION = 7;

   public static final int LEASE_IP_ADDRESS = 0;
   public static final int LEASE_MAC_ADDRESS = 1;
   public static final int LEASE_VENDOR = 2;
   public static final int LEASE_HOSTNAME = 3;
   public static final int LEASE_CLIENT_ID = 4;
   public static final int LEASE_SCOPE = 5;
   public static final int LEASE_TYPE = 6;
   public static final int LEASE_STATE = 7;
   public static final int LEASE_LAST_TRANSACTION = 8;
   public static final int LEASE_EXPIRATION = 9;

   private SortableTableViewer scopeViewer;
   private SortableTableViewer leaseViewer;
   private int leaseListUnavailableMessageId = 0;
   private Action actionCopyScopes;
   private Action actionCopyLeases;
   private Action actionExportScopes;
   private Action actionExportLeases;
   private Action actionExportAllLeases;
   private String selectedScopeId = null;
   private boolean scopeInputUpdate = false;

   /**
    * Constructor
    */
   public DhcpView()
   {
      super(LocalizationHelper.getI18n(DhcpView.class).tr("DHCP"), ResourceManager.getImageDescriptor("icons/object-views/address_map.png"), "objects.dhcp", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 157;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context instanceof AbstractNode) && ((AbstractNode)context).isDhcpServer();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      SashForm splitter = new SashForm(parent, SWT.VERTICAL);

      final String[] scopeNames = { i18n.tr("Network"), i18n.tr("Name"), i18n.tr("Ranges"), i18n.tr("Total"), i18n.tr("Used"), i18n.tr("Declined"), i18n.tr("Free"), i18n.tr("Utilization") };
      final int[] scopeWidths = { 180, 180, 300, 90, 90, 90, 90, 90 };
      scopeViewer = new SortableTableViewer(splitter, scopeNames, scopeWidths, SCOPE_NETWORK, SWT.UP, SWT.FULL_SELECTION | SWT.SINGLE, "DhcpView.Scopes");
      scopeViewer.setContentProvider(new ArrayContentProvider());
      scopeViewer.setLabelProvider(new DhcpScopeLabelProvider());
      scopeViewer.setComparator(new DhcpScopeComparator());
      scopeViewer.addSelectionChangedListener((event) -> {
         if (scopeInputUpdate)
            return; // selection is restored by restoreScopeSelection() after input change
         DhcpScope scope = (DhcpScope)scopeViewer.getStructuredSelection().getFirstElement();
         String scopeId = (scope != null) ? scope.getId() : null;
         if ((scopeId == null) ? (selectedScopeId != null) : !scopeId.equals(selectedScopeId))
         {
            selectedScopeId = scopeId;
            refreshLeases();
         }
      });

      final String[] leaseNames = { i18n.tr("IP Address"), i18n.tr("MAC Address"), i18n.tr("NIC Vendor"), i18n.tr("Host Name"), i18n.tr("Client ID"), i18n.tr("Scope"),
            i18n.tr("Type"), i18n.tr("State"), i18n.tr("Last Transaction"), i18n.tr("Expiration") };
      final int[] leaseWidths = { 180, 140, 160, 180, 200, 160, 100, 90, 150, 150 };
      leaseViewer = new SortableTableViewer(splitter, leaseNames, leaseWidths, LEASE_IP_ADDRESS, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI, "DhcpView.Leases");
      leaseViewer.setContentProvider(new ArrayContentProvider());
      leaseViewer.setLabelProvider(new DhcpLeaseLabelProvider(leaseViewer));
      leaseViewer.setComparator(new DhcpLeaseComparator());
      DhcpLeaseFilter filter = new DhcpLeaseFilter();
      leaseViewer.addFilter(filter);
      setFilterClient(leaseViewer, filter);

      splitter.setWeights(new int[] { 35, 65 });

      createActions();
      createContextMenu(scopeViewer.getControl(), true);
      createContextMenu(leaseViewer.getControl(), false);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionCopyScopes = new CopyTableRowsAction(scopeViewer, true);
      actionCopyLeases = new CopyTableRowsAction(leaseViewer, true);
      actionExportScopes = new ExportToCsvAction(this, scopeViewer, false);
      actionExportLeases = new ExportToCsvAction(this, leaseViewer, true);
      actionExportAllLeases = new ExportToCsvAction(this, leaseViewer, false);
   }

   /**
    * Create context menu for given control
    *
    * @param control control to attach menu to
    * @param scopes true for scope list, false for lease list
    */
   private void createContextMenu(Control control, boolean scopes)
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> {
         if (scopes)
         {
            m.add(actionCopyScopes);
            m.add(actionExportScopes);
         }
         else
         {
            m.add(actionCopyLeases);
            m.add(actionExportLeases);
            m.add(actionExportAllLeases);
         }
      });
      Menu menu = menuMgr.createContextMenu(control);
      control.setMenu(menu);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExportScopes);
      manager.add(new Separator());
      manager.add(actionExportAllLeases);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExportAllLeases);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      selectedScopeId = null;
      scopeViewer.setInput(new Object[0]);
      leaseViewer.setInput(new Object[0]);
      if (isActive())
         refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      final long objectId = getObjectId();
      if (objectId == 0)
         return;

      clearMessages();
      leaseListUnavailableMessageId = 0;
      new Job(i18n.tr("Reading DHCP scopes"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<DhcpScope> scopes = session.getDhcpScopes(objectId);
            runInUIThread(() -> {
               scopeInputUpdate = true;
               try
               {
                  scopeViewer.setInput(scopes.toArray());
               }
               finally
               {
                  scopeInputUpdate = false;
               }
               restoreScopeSelection(scopes);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read DHCP scopes");
         }
      }.start();
      refreshLeases();
   }

   /**
    * Restore scope selection after scope list refresh
    *
    * @param scopes new scope list
    */
   private void restoreScopeSelection(List<DhcpScope> scopes)
   {
      if (selectedScopeId == null)
         return;
      for(DhcpScope s : scopes)
      {
         if (s.getId().equals(selectedScopeId))
         {
            scopeViewer.setSelection(new StructuredSelection(s));
            return;
         }
      }
      selectedScopeId = null;
      refreshLeases();
   }

   /**
    * Refresh lease list for currently selected scope (or all scopes if none selected)
    */
   private void refreshLeases()
   {
      final long objectId = getObjectId();
      if (objectId == 0)
         return;

      final String scopeId = selectedScopeId;
      new Job(i18n.tr("Reading DHCP leases"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<DhcpLease> leases;
            try
            {
               leases = session.getDhcpLeases(objectId, scopeId);
            }
            catch(NXCException e)
            {
               // Device provides scope information but no lease list (for example, SNMP-only network device)
               if (e.getErrorCode() != RCC.DCI_NOT_SUPPORTED)
                  throw e;
               runInUIThread(() -> {
                  leaseViewer.setInput(new Object[0]);
                  if (leaseListUnavailableMessageId == 0)
                     leaseListUnavailableMessageId = addMessage(MessageArea.INFORMATION, i18n.tr("Lease list is not available for this DHCP server"));
               });
               return;
            }
            runInUIThread(() -> {
               if ((scopeId == null) ? (selectedScopeId == null) : scopeId.equals(selectedScopeId))
                  leaseViewer.setInput(leases.toArray());
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read DHCP leases");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      leaseViewer.getControl().setFocus();
   }
}
