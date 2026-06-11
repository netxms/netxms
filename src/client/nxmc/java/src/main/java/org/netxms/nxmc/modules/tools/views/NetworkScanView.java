/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.tools.views;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.InetAddressListElement;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.NetworkScanListener;
import org.netxms.client.constants.RCC;
import org.netxms.client.NetworkScanResult;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.snmp.SnmpVersion;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ZoneSelector;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Interactive network range scan view. Lets the user probe an IP range, view
 * detected hosts as results stream in, and bulk-create node objects from the
 * selection.
 */
public class NetworkScanView extends View
{
   private final I18n i18n = LocalizationHelper.getI18n(NetworkScanView.class);

   private static final String[] COLUMN_NAMES = { "IP Address", "RTT (ms)", "Agent", "SNMP", "Modbus", "EtherNet/IP", "Open TCP ports", "Node", "Node creation status" };
   private static final int[] COLUMN_WIDTHS = { 130, 80, 60, 80, 60, 80, 200, 150, 200 };
   private static final int COLUMN_IP = 0;
   private static final int COLUMN_RTT = 1;
   private static final int COLUMN_AGENT = 2;
   private static final int COLUMN_SNMP = 3;
   private static final int COLUMN_MODBUS = 4;
   private static final int COLUMN_ETHERNET_IP = 5;
   private static final int COLUMN_PORTS = 6;
   private static final int COLUMN_NODE = 7;
   private static final int COLUMN_ADD_STATUS = 8;

   private LabeledText startAddressEditor;
   private LabeledText endAddressEditor;
   private LabeledText tcpPortsEditor;
   private ZoneSelector zoneSelector;
   private Button probeAgentButton;
   private Button probeSnmpButton;
   private Button probeModbusButton;
   private Button probeEtherNetIpButton;
   private Button scanButton;
   private SortableTableViewer viewer;
   private Label statusLabel;

   private boolean zoningEnabled;
   private volatile boolean scanInProgress;
   private int scanZoneUIN;

   private final Map<String, ScanRow> rowIndex = new LinkedHashMap<>();
   private final List<ScanRow> rows = new ArrayList<>();

   private Action actionStartScan;
   private Action actionClearResults;
   private Action actionAddAsNodes;
   private Action actionExportToCSV;
   private Action actionExportAllToCSV;

   private InetAddress initialStart;
   private InetAddress initialEnd;
   private int initialZoneUIN;

   public NetworkScanView()
   {
      this(null, null, 0);
   }

   /**
    * Create the view with pre-set range and zone (for "Scan subnet..." from a Subnet context menu).
    *
    * @param start initial start address, or null to leave empty
    * @param end initial end address, or null to leave empty
    * @param zoneUIN initial zone UIN (ignored if zoning is disabled or zone not found)
    */
   public NetworkScanView(InetAddress start, InetAddress end, int zoneUIN)
   {
      super(LocalizationHelper.getI18n(NetworkScanView.class).tr("Network Scan"),
            ResourceManager.getImageDescriptor("icons/tool-views/search_history.png"), "tools.network-scan", false);
      this.initialStart = start;
      this.initialEnd = end;
      this.initialZoneUIN = zoneUIN;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      parent.setLayout(layout);

      final Composite searchBar = new Composite(parent, SWT.NONE);
      GridLayout barLayout = new GridLayout();
      barLayout.numColumns = 5;
      searchBar.setLayout(barLayout);
      searchBar.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      zoningEnabled = Registry.getSession().isZoningEnabled();
      if (zoningEnabled)
      {
         zoneSelector = new ZoneSelector(searchBar, SWT.NONE, false);
         zoneSelector.setLabel(i18n.tr("Zone"));
         GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
         gd.horizontalSpan = 5;
         zoneSelector.setLayoutData(gd);
      }

      startAddressEditor = new LabeledText(searchBar, SWT.NONE);
      startAddressEditor.setLabel(i18n.tr("Start address"));
      startAddressEditor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      endAddressEditor = new LabeledText(searchBar, SWT.NONE);
      endAddressEditor.setLabel(i18n.tr("End address"));
      endAddressEditor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      tcpPortsEditor = new LabeledText(searchBar, SWT.NONE);
      tcpPortsEditor.setLabel(i18n.tr("TCP ports (comma-separated)"));
      tcpPortsEditor.setText("22, 80, 443");
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      tcpPortsEditor.setLayoutData(gd);

      scanButton = new Button(searchBar, SWT.PUSH);
      scanButton.setImage(SharedIcons.IMG_EXECUTE);
      scanButton.setText(i18n.tr("Scan"));
      scanButton.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, false, false));
      scanButton.addListener(SWT.Selection, e -> doScan());

      Composite probeBar = new Composite(searchBar, SWT.NONE);
      GridLayout probeLayout = new GridLayout(4, false);
      probeLayout.marginHeight = 0;
      probeLayout.marginWidth = 0;
      probeBar.setLayout(probeLayout);
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 5;
      probeBar.setLayoutData(gd);

      probeAgentButton = new Button(probeBar, SWT.CHECK);
      probeAgentButton.setText(i18n.tr("Probe NetXMS agent"));
      probeAgentButton.setSelection(true);

      probeSnmpButton = new Button(probeBar, SWT.CHECK);
      probeSnmpButton.setText(i18n.tr("Probe SNMP"));
      probeSnmpButton.setSelection(true);

      probeModbusButton = new Button(probeBar, SWT.CHECK);
      probeModbusButton.setText(i18n.tr("Probe Modbus/TCP"));
      probeModbusButton.setSelection(false);

      probeEtherNetIpButton = new Button(probeBar, SWT.CHECK);
      probeEtherNetIpButton.setText(i18n.tr("Probe EtherNet/IP"));
      probeEtherNetIpButton.setSelection(false);

      new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      statusLabel = new Label(parent, SWT.NONE);
      statusLabel.setFont(JFaceResources.getBannerFont());
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.verticalIndent = 8;
      gd.horizontalIndent = 4;
      statusLabel.setLayoutData(gd);
      setStatusText(i18n.tr("Scan is not running"));

      Label separator = new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL);
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.verticalIndent = 8;
      separator.setLayoutData(gd);

      viewer = new SortableTableViewer(parent, COLUMN_NAMES, COLUMN_WIDTHS, COLUMN_IP, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ResultLabelProvider());
      viewer.setInput(rows);
      viewer.getControl().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      viewer.addSelectionChangedListener(e -> actionAddAsNodes.setEnabled(!scanInProgress && !viewer.getStructuredSelection().isEmpty()));

      if (initialStart != null)
         startAddressEditor.setText(initialStart.getHostAddress());
      if (initialEnd != null)
         endAddressEditor.setText(initialEnd.getHostAddress());
      if (zoningEnabled && (zoneSelector != null) && (Registry.getSession().findZone(initialZoneUIN) != null))
         zoneSelector.setZoneUIN(initialZoneUIN);

      createActions();
      createContextMenu();
   }

   /**
    * Create context menu for results table.
    */
   private void createContextMenu()
   {
      MenuManager manager = new MenuManager();
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener(m -> {
         m.add(actionAddAsNodes);
         m.add(new Separator());
         m.add(actionExportToCSV);
         m.add(actionExportAllToCSV);
         m.add(new Separator());
         m.add(actionClearResults);
      });
      viewer.getControl().setMenu(manager.createContextMenu(viewer.getControl()));
   }

   /**
    * Build view actions.
    */
   private void createActions()
   {
      actionStartScan = new Action(i18n.tr("&Scan"), SharedIcons.EXECUTE) {
         @Override
         public void run()
         {
            doScan();
         }
      };
      addKeyBinding("F9", actionStartScan);

      actionClearResults = new Action(i18n.tr("&Clear results"), SharedIcons.CLEAR) {
         @Override
         public void run()
         {
            clearResults();
         }
      };

      actionAddAsNodes = new Action(i18n.tr("&Add selected as nodes...")) {
         @Override
         public void run()
         {
            addSelectedAsNodes();
         }
      };
      actionAddAsNodes.setEnabled(false);

      actionExportToCSV = new ExportToCsvAction(this, viewer, true);
      actionExportAllToCSV = new ExportToCsvAction(this, viewer, false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionStartScan);
      manager.add(new Separator());
      manager.add(actionExportAllToCSV);
      manager.add(new Separator());
      manager.add(actionClearResults);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionStartScan);
      manager.add(new Separator());
      manager.add(actionExportAllToCSV);
      manager.add(new Separator());
      manager.add(actionClearResults);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      if (startAddressEditor != null)
         startAddressEditor.setFocus();
   }

   /**
    * Update the status line.
    */
   private void setStatusText(String text)
   {
      if ((statusLabel != null) && !statusLabel.isDisposed())
         statusLabel.setText(text);
   }

   /**
    * Parse the comma-separated TCP port list. Invalid tokens are silently
    * skipped; an empty result is returned as an empty array.
    */
   private int[] parseTcpPorts()
   {
      String text = tcpPortsEditor.getText().trim();
      if (text.isEmpty())
         return new int[0];
      String[] tokens = text.split(",");
      List<Integer> values = new ArrayList<>(tokens.length);
      for(String t : tokens)
      {
         String trimmed = t.trim();
         if (trimmed.isEmpty())
            continue;
         try
         {
            int port = Integer.parseInt(trimmed);
            if ((port > 0) && (port <= 65535))
               values.add(port);
         }
         catch(NumberFormatException e)
         {
            // skip silently
         }
      }
      int[] result = new int[values.size()];
      for(int i = 0; i < values.size(); i++)
         result[i] = values.get(i);
      return result;
   }

   /**
    * Kick off the scan.
    */
   private void doScan()
   {
      if (scanInProgress)
         return;

      final InetAddress start;
      final InetAddress end;
      try
      {
         start = InetAddress.getByName(startAddressEditor.getText().trim());
         end = InetAddress.getByName(endAddressEditor.getText().trim());
      }
      catch(UnknownHostException e)
      {
         MessageDialogHelper.openWarning(getWindow().getShell(), i18n.tr("Warning"), i18n.tr("Please enter valid start and end IP addresses"));
         return;
      }

      final int zoneUIN = (zoningEnabled && (zoneSelector != null)) ? zoneSelector.getZoneUIN() : 0;
      scanZoneUIN = zoneUIN;
      final int[] tcpPorts = parseTcpPorts();

      int flags = 0;
      if (probeAgentButton.getSelection())
         flags |= NetworkScanResult.PROBE_AGENT;
      if (probeSnmpButton.getSelection())
         flags |= NetworkScanResult.PROBE_SNMP;
      if (probeModbusButton.getSelection())
         flags |= NetworkScanResult.PROBE_MODBUS;
      if (probeEtherNetIpButton.getSelection())
         flags |= NetworkScanResult.PROBE_ETHERNET_IP;
      final int scanFlags = flags;

      clearResults();
      setScanInProgress(true);
      setStatusText(i18n.tr("Scanning..."));

      final InetAddressListElement range = new InetAddressListElement(start, end, zoneUIN, 0, "");
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Scanning network range"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            NetworkScanListener listener = new NetworkScanListener() {
               @Override
               public void resultReceived(final NetworkScanResult result)
               {
                  runInUIThread(() -> handleResult(result));
               }

               @Override
               public void scanCompletedWithWarning(final int rcc)
               {
                  final String message = RCC.getText(rcc, Locale.getDefault().getLanguage(), null);
                  runInUIThread(() -> addMessage(MessageArea.WARNING, message));
               }
            };
            final int reported = session.scanNetworkRange(range, tcpPorts, scanFlags, listener);
            runInUIThread(() -> {
               setScanInProgress(false);
               setStatusText(String.format(i18n.tr("Scan complete: %d host(s) reported"), reported));
            });
         }

         @Override
         protected void jobFailureHandler(Exception e)
         {
            runInUIThread(() -> {
               setScanInProgress(false);
               setStatusText(i18n.tr("Scan failed"));
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Network range scan failed");
         }
      }.start();
   }

   /**
    * Toggle controls based on whether a scan is in progress.
    */
   private void setScanInProgress(boolean inProgress)
   {
      scanInProgress = inProgress;
      if ((scanButton != null) && !scanButton.isDisposed())
         scanButton.setEnabled(!inProgress);
      actionStartScan.setEnabled(!inProgress);
      actionAddAsNodes.setEnabled(!inProgress && (viewer != null) && !viewer.getStructuredSelection().isEmpty());
   }

   /**
    * Merge one streamed result into the table.
    */
   private void handleResult(NetworkScanResult result)
   {
      if (result.getAddress() == null)
         return;
      String key = result.getAddress().getHostAddress();
      ScanRow row = rowIndex.get(key);
      if (row == null)
      {
         row = new ScanRow(result);
         rowIndex.put(key, row);
         rows.add(row);
         viewer.add(row);
      }
      else
      {
         row.result = result;
         viewer.update(row, null);
      }
      setStatusText(String.format(i18n.tr("Scanning... %d host(s) reported"), rows.size()));
   }

   /**
    * Drop all results from the table.
    */
   private void clearResults()
   {
      rowIndex.clear();
      rows.clear();
      if (viewer != null)
         viewer.refresh();
      setStatusText(i18n.tr("Ready"));
   }

   /**
    * Bulk-create node objects for the selected rows.
    */
   private void addSelectedAsNodes()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      final List<ScanRow> targets = new ArrayList<>();
      final List<ScanRow> existing = new ArrayList<>();
      for(Iterator<?> it = selection.iterator(); it.hasNext(); )
      {
         ScanRow row = (ScanRow)it.next();
         if ((row.result.getNodeId() != 0) || (row.createdObjectId != 0))
            existing.add(row);
         else
            targets.add(row);
      }

      if (targets.isEmpty())
      {
         for(ScanRow row : existing)
         {
            row.addStatus = AddStatus.ALREADY_EXISTS;
            viewer.update(row, null);
         }
         addMessage(MessageArea.INFORMATION, i18n.tr("All selected addresses already have node objects"));
         return;
      }

      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getWindow().getShell(), ObjectSelectionDialog.createContainerSelectionFilter());
      if (dlg.open() != Window.OK)
         return;
      List<AbstractObject> selectedParents = dlg.getSelectedObjects();
      if (selectedParents.isEmpty())
         return;
      final long parentId = selectedParents.get(0).getObjectId();

      final int zoneUIN = scanZoneUIN;
      final NXCSession session = Registry.getSession();
      for(ScanRow row : existing)
      {
         row.addStatus = AddStatus.ALREADY_EXISTS;
         viewer.update(row, null);
      }
      for(ScanRow row : targets)
      {
         row.addStatus = AddStatus.PENDING;
         viewer.update(row, null);
      }

      new Job(i18n.tr("Adding scanned hosts as nodes"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(final ScanRow row : targets)
            {
               runInUIThread(() -> {
                  row.addStatus = AddStatus.CREATING;
                  viewer.update(row, null);
               });
               try
               {
                  NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE,
                        row.result.getAddress().getHostAddress(), parentId);
                  cd.setPrimaryName(row.result.getAddress().getHostAddress());
                  cd.setZoneUIN(zoneUIN);
                  if (row.result.getAgentPort() != 0)
                     cd.setAgentPort(row.result.getAgentPort());
                  final long createdId = session.createObject(cd);
                  runInUIThread(() -> {
                     row.addStatus = AddStatus.SUCCESS;
                     row.createdObjectId = createdId;
                     viewer.update(row, null);
                  });
               }
               catch(Exception ex)
               {
                  final String err = ex.getLocalizedMessage();
                  runInUIThread(() -> {
                     row.addStatus = AddStatus.FAILED;
                     row.errorMessage = err;
                     viewer.update(row, null);
                  });
               }
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Bulk node creation failed");
         }
      }.start();
   }

   /**
    * Per-row record in the scan results table.
    */
   private static class ScanRow
   {
      NetworkScanResult result;
      AddStatus addStatus;
      long createdObjectId;
      String errorMessage;

      ScanRow(NetworkScanResult result)
      {
         this.result = result;
         this.addStatus = AddStatus.NONE;
      }
   }

   /**
    * Outcome of attempting to bulk-create a node from a scan row.
    */
   private enum AddStatus
   {
      NONE, PENDING, CREATING, SUCCESS, FAILED, ALREADY_EXISTS
   }

   /**
    * Label provider for the result table.
    */
   private class ResultLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         ScanRow row = (ScanRow)element;
         NetworkScanResult r = row.result;
         switch(columnIndex)
         {
            case COLUMN_IP:
               return r.getAddress().getHostAddress();
            case COLUMN_RTT:
               return r.isIcmpReachable() ? Long.toString(r.getRtt()) : "";
            case COLUMN_AGENT:
               return r.isAgentDetected() ? Integer.toString(r.getAgentPort()) : "";
            case COLUMN_SNMP:
               if (!r.isSnmpDetected())
                  return "";
               SnmpVersion v = r.getSnmpVersion();
               return (v != null) ? v.name() : "";
            case COLUMN_MODBUS:
               return r.isModbusDetected() ? i18n.tr("Yes") : "";
            case COLUMN_ETHERNET_IP:
               return r.isEtherNetIpDetected() ? i18n.tr("Yes") : "";
            case COLUMN_PORTS:
               int[] ports = r.getOpenTcpPorts();
               if ((ports == null) || (ports.length == 0))
                  return "";
               StringBuilder sb = new StringBuilder();
               int[] sorted = Arrays.copyOf(ports, ports.length);
               Arrays.sort(sorted);
               for(int i = 0; i < sorted.length; i++)
               {
                  if (i > 0)
                     sb.append(", ");
                  sb.append(sorted[i]);
               }
               return sb.toString();
            case COLUMN_NODE:
               long nodeId = (row.createdObjectId != 0) ? row.createdObjectId : r.getNodeId();
               return (nodeId != 0) ? Registry.getSession().getObjectName(nodeId) : "";
            case COLUMN_ADD_STATUS:
               return formatAddStatus(row);
         }
         return "";
      }

      private String formatAddStatus(ScanRow row)
      {
         switch(row.addStatus)
         {
            case NONE:
               return "";
            case PENDING:
               return i18n.tr("pending");
            case CREATING:
               return i18n.tr("creating...");
            case SUCCESS:
               return String.format(i18n.tr("created [%d]"), row.createdObjectId);
            case FAILED:
               return String.format(i18n.tr("failed: %s"), (row.errorMessage != null) ? row.errorMessage : i18n.tr("unknown error"));
            case ALREADY_EXISTS:
               return i18n.tr("node already exists");
         }
         return "";
      }
   }
}
