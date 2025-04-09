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
package org.netxms.nxmc.modules.serverconfig.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.InetAddressListElement;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.NetworkDiscoveryFilterFlags;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.Section;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptSelector;
import org.netxms.nxmc.modules.serverconfig.dialogs.AddressListElementEditDialog;
import org.netxms.nxmc.modules.serverconfig.views.helpers.AddressListElementComparator;
import org.netxms.nxmc.modules.serverconfig.views.helpers.AddressListLabelProvider;
import org.netxms.nxmc.modules.serverconfig.views.helpers.NetworkDiscoveryConfig;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Configurator for network discovery
 */
public class NetworkDiscoveryConfigurator extends ConfigurationView
{
   public static final int RANGE = 0;
   public static final int ZONE = 1;
   public static final int PROXY = 2;
   public static final int COMMENTS = 3;

   private I18n i18n = LocalizationHelper.getI18n(NetworkDiscoveryConfigurator.class);

   private NetworkDiscoveryConfig config;
   private boolean modified = false;
   private Composite content;
   private Button radioDiscoveryOff;
   private Button radioDiscoveryPassive;
   private Button radioDiscoveryActive;
   private Button radioDiscoveryActiveAndPassive;
   private Button checkEnableSNMPProbing;
   private Button checkEnableTCPProbing;
   private Button checkUseSnmpTraps;
   private Button checkUseSyslog;
   private Spinner passiveDiscoveryInterval;
   private Label activeDiscoveryScheduleLabel;
   private Button radioActiveDiscoveryInterval;
   private Button radioActiveDiscoverySchedule;
   private Spinner activeDiscoveryInterval;
   private LabeledText activeDiscoverySchedule;
   private Button checkFilterRange;
   private Button checkFilterProtocols;
   private Button checkAllowAgent;
   private Button checkAllowSNMP;
   private Button checkAllowSSH;
   private Button checkFilterScript;
   private ScriptSelector filterScript;
   private SortableTableViewer filterAddressList;
   private SortableTableViewer activeDiscoveryAddressList;
   private Action actionSave;

   /**
    * Create network discovery configuration view
    */
   public NetworkDiscoveryConfigurator()
   {
      super(LocalizationHelper.getI18n(NetworkDiscoveryConfigurator.class).tr("Network Discovery"), ResourceManager.getImageDescriptor("icons/config-views/network_credentials.png"),
            "config.network-discovery", false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      ScrolledComposite scroller = new ScrolledComposite(parent, SWT.V_SCROLL);
      scroller.setExpandHorizontal(true);
      scroller.setExpandVertical(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            content.layout(true, true);
            scroller.setMinSize(content.computeSize(scroller.getSize().x, SWT.DEFAULT));
         }
      });

      content = new Composite(scroller, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      content.setLayout(layout);
      content.setBackground(content.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

      scroller.setContent(content);

      createGeneralSection();
      createFilterSection();
      createScheduleSection();
      createActiveDiscoverySection();

      createActions();

      // Restoring, load config
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Loading network discovery configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final NetworkDiscoveryConfig loadedConfig = NetworkDiscoveryConfig.load(session);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  setConfig(loadedConfig);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load network discovery configuration");
         }
      }.start();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionSave = new Action(i18n.tr("&Save"), SharedIcons.SAVE) {
         @Override
         public void run()
         {
            save();
         }
      };
      addKeyBinding("M1+S", actionSave);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionSave);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionSave);
   }

   /**
    * Create "General" section
    */
   private void createGeneralSection()
   {
      Section section = new Section(content, i18n.tr("General"), false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
      clientArea.setBackground(content.getBackground());

      GridLayout layout = new GridLayout();
      clientArea.setLayout(layout);

      final SelectionListener listener = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            setModified();
            if (radioDiscoveryOff.getSelection())
            {
               config.setDiscoveryType(NetworkDiscoveryConfig.DISCOVERY_TYPE_NONE);
               enableActiveDiscovery(false);
               enablePassiveDiscovery(false);
            }
            else if (radioDiscoveryPassive.getSelection())
            {
               config.setDiscoveryType(NetworkDiscoveryConfig.DISCOVERY_TYPE_PASSIVE);
               enableActiveDiscovery(false);
               enablePassiveDiscovery(true);
            }
            else if (radioDiscoveryActive.getSelection())
            {
               config.setDiscoveryType(NetworkDiscoveryConfig.DISCOVERY_TYPE_ACTIVE);
               enableActiveDiscovery(true);
               enablePassiveDiscovery(false);
            }
            else if (radioDiscoveryActiveAndPassive.getSelection())
            {
               config.setDiscoveryType(NetworkDiscoveryConfig.DISCOVERY_TYPE_ACTIVE_PASSIVE);
               enableActiveDiscovery(true);
               enablePassiveDiscovery(true);
            }
         }
      };

      radioDiscoveryOff = new Button(clientArea, SWT.RADIO);
      radioDiscoveryOff.setText(i18n.tr("Disabled"));
      radioDiscoveryOff.setBackground(clientArea.getBackground());
      radioDiscoveryOff.addSelectionListener(listener);
      radioDiscoveryPassive = new Button(clientArea, SWT.RADIO);
      radioDiscoveryPassive.setText(i18n.tr("Passive only"));
      radioDiscoveryPassive.setBackground(clientArea.getBackground());
      radioDiscoveryPassive.addSelectionListener(listener);
      radioDiscoveryActive = new Button(clientArea, SWT.RADIO);
      radioDiscoveryActive.setText(i18n.tr("Active only"));
      radioDiscoveryActive.setBackground(clientArea.getBackground());
      radioDiscoveryActive.addSelectionListener(listener);
      radioDiscoveryActiveAndPassive = new Button(clientArea, SWT.RADIO);
      radioDiscoveryActiveAndPassive.setText(i18n.tr("Active and passive"));
      radioDiscoveryActiveAndPassive.setBackground(clientArea.getBackground());
      radioDiscoveryActiveAndPassive.addSelectionListener(listener);      

      checkEnableSNMPProbing = new Button(clientArea, SWT.CHECK);
      checkEnableSNMPProbing.setText(i18n.tr("Enable SNMP probing"));
      checkEnableSNMPProbing.setBackground(clientArea.getBackground());
      checkEnableSNMPProbing.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            config.setEnableSNMPProbing(checkEnableSNMPProbing.getSelection());
            setModified();
         }
      });
      gd = new GridData();
      gd.verticalIndent = 10;
      checkEnableSNMPProbing.setLayoutData(gd);

      checkEnableTCPProbing = new Button(clientArea, SWT.CHECK);
      checkEnableTCPProbing.setText(i18n.tr("Enable TCP probing"));
      checkEnableTCPProbing.setBackground(clientArea.getBackground());
      checkEnableTCPProbing.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            config.setEnableTCPProbing(checkEnableTCPProbing.getSelection());
            setModified();
         }
      });

      checkUseSnmpTraps = new Button(clientArea, SWT.CHECK);
      checkUseSnmpTraps.setText(i18n.tr("Use SNMP trap source addresses for discovery"));
      checkUseSnmpTraps.setBackground(clientArea.getBackground());
      checkUseSnmpTraps.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            config.setUseSnmpTraps(checkUseSnmpTraps.getSelection());
            setModified();
         }
      });

      checkUseSyslog = new Button(clientArea, SWT.CHECK);
      checkUseSyslog.setText(i18n.tr("Use syslog source addresses for discovery"));
      checkUseSyslog.setBackground(clientArea.getBackground());
      checkUseSyslog.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            config.setUseSyslog(checkUseSyslog.getSelection());
            setModified();
         }
      });
   }

   /**
    * Enable/disable passive discovery
    *
    * @param enabled enable flag
    */
   private void enablePassiveDiscovery(boolean enabled)
   {
      passiveDiscoveryInterval.setEnabled(enabled);
   }

   /**
    * Enable/disable active discovery
    *
    * @param enabled enable flag
    */
   private void enableActiveDiscovery(boolean enabled)
   {
      if (radioActiveDiscoveryInterval.getSelection())
      {
         activeDiscoverySchedule.setEnabled(false);
      }
      else
      {
         activeDiscoveryInterval.setEnabled(false);
      }
      radioActiveDiscoveryInterval.setEnabled(enabled);
      radioActiveDiscoverySchedule.setEnabled(enabled);
      checkEnableSNMPProbing.setEnabled(enabled);
      checkEnableTCPProbing.setEnabled(enabled);
   }

   /**
    * Create "Schedule" section
    */
   private void createScheduleSection()
   {
      Section section = new Section(content, i18n.tr("Schedule"), false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
      clientArea.setBackground(content.getBackground());
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      passiveDiscoveryInterval = WidgetHelper.createLabeledSpinner(clientArea, SWT.BORDER, i18n.tr("Passive discovery interval"), 0, 0xffffff, gd);
      passiveDiscoveryInterval.setBackground(clientArea.getBackground());
      passiveDiscoveryInterval.getParent().setBackground(clientArea.getBackground());
      passiveDiscoveryInterval.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            try
            {
               config.setPassiveDiscoveryPollInterval(Integer.parseInt(passiveDiscoveryInterval.getText()));
               setModified();
            }
            catch(NumberFormatException ex)
            {
            }
         }
      });
      
      activeDiscoveryScheduleLabel = new Label(clientArea, SWT.LEFT);
      activeDiscoveryScheduleLabel.setText(i18n.tr("Active discovery schedule configuration"));
      activeDiscoveryScheduleLabel.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.horizontalSpan = 2;
      activeDiscoveryScheduleLabel.setLayoutData(gd);

      final SelectionListener listener = new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            setModified();
            if (radioActiveDiscoveryInterval.getSelection())
            {
               config.setActiveDiscoveryPollInterval(Integer.parseInt(activeDiscoveryInterval.getText()));  
               activeDiscoveryInterval.setSelection(config.getActiveDiscoveryPollInterval() == 0 ? NetworkDiscoveryConfig.DEFAULT_ACTIVE_INTERVAL : config.getActiveDiscoveryPollInterval());
               activeDiscoverySchedule.setEnabled(false);
               activeDiscoveryInterval.setEnabled(true);         
            }
            else
            {
               config.setActiveDiscoveryPollInterval(0);
               activeDiscoverySchedule.setText(config.getActiveDiscoveryPollSchedule());
               activeDiscoverySchedule.setEnabled(true);
               activeDiscoveryInterval.setEnabled(false); 
            }
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      };      

      radioActiveDiscoveryInterval = new Button(clientArea, SWT.RADIO);
      radioActiveDiscoveryInterval.setText(i18n.tr("Interval"));
      radioActiveDiscoveryInterval.setBackground(clientArea.getBackground());
      radioActiveDiscoveryInterval.addSelectionListener(listener);

      radioActiveDiscoverySchedule = new Button(clientArea, SWT.RADIO);
      radioActiveDiscoverySchedule.setText(i18n.tr("Schedule"));
      radioActiveDiscoverySchedule.setBackground(clientArea.getBackground());
      radioActiveDiscoverySchedule.addSelectionListener(listener);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.LEFT;
      radioActiveDiscoverySchedule.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      activeDiscoveryInterval = WidgetHelper.createLabeledSpinner(clientArea, SWT.BORDER, i18n.tr("Active discovery interval"), 0, 0xffffff, gd);
      activeDiscoveryInterval.setBackground(clientArea.getBackground());
      activeDiscoveryInterval.getParent().setBackground(clientArea.getBackground());
      activeDiscoveryInterval.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            config.setActiveDiscoveryPollInterval(Integer.parseInt(activeDiscoveryInterval.getText()));
            setModified();
         }
      });

      activeDiscoverySchedule = new LabeledText(clientArea, SWT.NONE, SWT.SINGLE | SWT.BORDER);
      activeDiscoverySchedule.setLabel(i18n.tr("Active discovery schedule"));
      activeDiscoverySchedule.setBackground(clientArea.getBackground());
      activeDiscoverySchedule.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            config.setActiveDiscoveryPollSchedule(activeDiscoverySchedule.getText());
            setModified();
         }
      });  
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.LEFT;
      activeDiscoverySchedule.setLayoutData(gd);  
   }

   /**
    * Create "Filter" section
    */
   private void createFilterSection()
   {
      Section section = new Section(content, i18n.tr("Filter"), false);
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalSpan = 3;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
      clientArea.setBackground(content.getBackground());

      GridLayout layout = new GridLayout();
      clientArea.setLayout(layout);

      final SelectionListener checkBoxListener = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            setModified();
            int flags = 0;
            if (checkFilterRange.getSelection())
               flags |= NetworkDiscoveryFilterFlags.CHECK_ADDRESS_RANGE;
            if (checkFilterScript.getSelection())
               flags |= NetworkDiscoveryFilterFlags.EXECUTE_SCRIPT;
            if (checkFilterProtocols.getSelection())
               flags |= NetworkDiscoveryFilterFlags.CHECK_PROTOCOLS;
            if (checkAllowAgent.getSelection())
               flags |= NetworkDiscoveryFilterFlags.PROTOCOL_AGENT;
            if (checkAllowSNMP.getSelection())
               flags |= NetworkDiscoveryFilterFlags.PROTOCOL_SNMP;
            if (checkAllowSSH.getSelection())
               flags |= NetworkDiscoveryFilterFlags.PROTOCOL_SSH;
            config.setFilterFlags(flags);

            boolean enableProtocolFilter = checkFilterProtocols.getSelection();
            checkAllowAgent.setEnabled(enableProtocolFilter);
            checkAllowSNMP.setEnabled(enableProtocolFilter);
            checkAllowSSH.setEnabled(enableProtocolFilter);
         }
      };

      checkFilterRange = new Button(clientArea, SWT.CHECK);
      checkFilterRange.setText(i18n.tr("By address range"));
      checkFilterRange.setBackground(clientArea.getBackground());
      checkFilterRange.addSelectionListener(checkBoxListener);

      Composite addressRangeEditor = new Composite(clientArea, SWT.NONE);
      addressRangeEditor.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.horizontalIndent = 20;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      addressRangeEditor.setLayoutData(gd);
      GridLayout addressRangeLayout = new GridLayout();
      addressRangeLayout.numColumns = 2;
      addressRangeEditor.setLayout(addressRangeLayout);

      final String[] names = { i18n.tr("Range"), i18n.tr("Comment") };
      final int[] widths = { 150, 150 };
      filterAddressList = new SortableTableViewer(addressRangeEditor, names, widths, 0, SWT.DOWN, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 3;
      gd.heightHint = 300;
      filterAddressList.getTable().setLayoutData(gd);
      filterAddressList.getTable().setSortDirection(SWT.UP);
      filterAddressList.setContentProvider(new ArrayContentProvider());
      filterAddressList.setLabelProvider(new AddressListLabelProvider(false));
      filterAddressList.setComparator(new AddressListElementComparator(false));
      filterAddressList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editAddressFilterElement();
         }
      });

      final ImageHyperlink linkAdd = new ImageHyperlink(addressRangeEditor, SWT.NONE);
      linkAdd.setText(i18n.tr("Add..."));
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addAddressFilterElement();
         }
      });

      final ImageHyperlink linkEdit = new ImageHyperlink(addressRangeEditor, SWT.NONE);
      linkEdit.setText(i18n.tr("Edit..."));
      linkEdit.setImage(SharedIcons.IMG_EDIT);
      linkEdit.setBackground(clientArea.getBackground());
      linkEdit.setEnabled(false);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkEdit.setLayoutData(gd);
      linkEdit.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            editAddressFilterElement();
         }
      });

      final ImageHyperlink linkRemove = new ImageHyperlink(addressRangeEditor, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
      linkRemove.setEnabled(false);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkRemove.setLayoutData(gd);
      linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            removeAddressFilterElements();
         }
      });

      filterAddressList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = filterAddressList.getStructuredSelection();
            linkEdit.setEnabled(selection.size() == 1);
            linkRemove.setEnabled(!selection.isEmpty());
         }
      });

      checkFilterProtocols = new Button(clientArea, SWT.CHECK);
      checkFilterProtocols.setText(i18n.tr("By communication protocols"));
      checkFilterProtocols.setBackground(clientArea.getBackground());
      checkFilterProtocols.addSelectionListener(checkBoxListener);

      checkAllowAgent = new Button(clientArea, SWT.CHECK);
      checkAllowAgent.setText(i18n.tr("Accept node if it has &NetXMS agent"));
      checkAllowAgent.setBackground(clientArea.getBackground());
      checkAllowAgent.addSelectionListener(checkBoxListener);
      gd = new GridData();
      gd.horizontalIndent = 20;
      checkAllowAgent.setLayoutData(gd);

      checkAllowSNMP = new Button(clientArea, SWT.CHECK);
      checkAllowSNMP.setText(i18n.tr("Accept node if it has &SNMP agent"));
      checkAllowSNMP.setBackground(clientArea.getBackground());
      checkAllowSNMP.addSelectionListener(checkBoxListener);
      gd = new GridData();
      gd.horizontalIndent = 20;
      checkAllowSNMP.setLayoutData(gd);

      checkAllowSSH = new Button(clientArea, SWT.CHECK);
      checkAllowSSH.setText(i18n.tr("Accept node if it is accessible via SS&H"));
      checkAllowSSH.setBackground(clientArea.getBackground());
      checkAllowSSH.addSelectionListener(checkBoxListener);
      gd = new GridData();
      gd.horizontalIndent = 20;
      checkAllowSSH.setLayoutData(gd);

      checkFilterScript = new Button(clientArea, SWT.CHECK);
      checkFilterScript.setText(i18n.tr("With custom script"));
      checkFilterScript.setBackground(clientArea.getBackground());
      checkFilterScript.addSelectionListener(checkBoxListener);

      filterScript = new ScriptSelector(clientArea, SWT.NONE, true, false);
      filterScript.setBackground(clientArea.getBackground());
      filterScript.getTextControl().setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalIndent = 20;
      filterScript.setLayoutData(gd);
      filterScript.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            config.setFilterScript(filterScript.getScriptName());
            setModified();
         }
      });
   }

   /**
    * Create "Active Discovery Targets" section
    */
   private void createActiveDiscoverySection()
   {
      Section section = new Section(content, i18n.tr("Active discovery targets"), false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      section.setLayoutData(gd);

      Composite clientArea = section.getClient();
      clientArea.setBackground(content.getBackground());
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
      clientArea.setLayout(layout);

      final String[] names = { i18n.tr("Range"), i18n.tr("Zone"), i18n.tr("Proxy"), i18n.tr("Comments") };
      final int[] widths = { 150, 150, 150, 150 };
      activeDiscoveryAddressList = new SortableTableViewer(clientArea, names, widths, 0, SWT.DOWN, SWT.MULTI | SWT.FULL_SELECTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 200;
      activeDiscoveryAddressList.getTable().setLayoutData(gd);
      activeDiscoveryAddressList.getTable().setSortDirection(SWT.UP);
      activeDiscoveryAddressList.setContentProvider(new ArrayContentProvider());
      activeDiscoveryAddressList.setLabelProvider(new AddressListLabelProvider(true));
      activeDiscoveryAddressList.setComparator(new AddressListElementComparator(true));
      
      // Hide Zone column if zoning is disabled
      if (!Registry.getSession().isZoningEnabled())
      {
         final org.eclipse.swt.widgets.TableColumn zoneColumn = activeDiscoveryAddressList.getTable().getColumn(ZONE);
         zoneColumn.setWidth(0);
         zoneColumn.setResizable(false);
      }
      activeDiscoveryAddressList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editTargetAddressListElement();
         }
      });

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
      linkAdd.setText(i18n.tr("Add..."));
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addTargetAddressListElement();
         }
      });      

      final ImageHyperlink linkEdit = new ImageHyperlink(controlArea, SWT.NONE);
      linkEdit.setText(i18n.tr("Edit..."));
      linkEdit.setImage(SharedIcons.IMG_EDIT);
      linkEdit.setBackground(clientArea.getBackground());
      linkEdit.setEnabled(false);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkEdit.setLayoutData(gd);
      linkEdit.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            editTargetAddressListElement();
         }
      });

      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
      linkRemove.setEnabled(false);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkRemove.setLayoutData(gd);
      linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            removeTargetAddressListElements();
         }
      });

      final ImageHyperlink runActiveDiscovery = new ImageHyperlink(controlArea, SWT.NONE);
      runActiveDiscovery.setText(i18n.tr("Scan"));
      runActiveDiscovery.setToolTipText(i18n.tr("Runs active discovery on selected ranges"));
      runActiveDiscovery.setImage(SharedIcons.IMG_EXECUTE);
      runActiveDiscovery.setBackground(clientArea.getBackground());
      runActiveDiscovery.setEnabled(false);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      runActiveDiscovery.setLayoutData(gd);
      runActiveDiscovery.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            scanAddressRange();
         }
      });

      activeDiscoveryAddressList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = activeDiscoveryAddressList.getStructuredSelection();
            linkEdit.setEnabled(selection.size() == 1);
            linkRemove.setEnabled(!selection.isEmpty());
            runActiveDiscovery.setEnabled(!selection.isEmpty());
         }
      });
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      content.setFocus();
   }

   /**
    * @param config
    */
   public void setConfig(NetworkDiscoveryConfig config)
   {
      this.config = config;

      radioDiscoveryOff.setSelection(config.getDiscoveryType() == NetworkDiscoveryConfig.DISCOVERY_TYPE_NONE);
      radioDiscoveryPassive.setSelection(config.getDiscoveryType() == NetworkDiscoveryConfig.DISCOVERY_TYPE_PASSIVE);
      radioDiscoveryActive.setSelection(config.getDiscoveryType() == NetworkDiscoveryConfig.DISCOVERY_TYPE_ACTIVE);
      radioDiscoveryActiveAndPassive.setSelection(config.getDiscoveryType() == NetworkDiscoveryConfig.DISCOVERY_TYPE_ACTIVE_PASSIVE);
      checkEnableSNMPProbing.setSelection(config.isEnableSNMPProbing());
      checkEnableTCPProbing.setSelection(config.isEnableTCPProbing());
      checkUseSnmpTraps.setSelection(config.isUseSnmpTraps());
      checkUseSyslog.setSelection(config.isUseSyslog());

      passiveDiscoveryInterval.setSelection(config.getPassiveDiscoveryPollInterval());
      if(config.getActiveDiscoveryPollInterval() != 0)
      {
         radioActiveDiscoveryInterval.setSelection(true);
      }
      else
      {
         radioActiveDiscoverySchedule.setSelection(true);
      }
      activeDiscoveryInterval.setSelection(config.getActiveDiscoveryPollInterval());
      activeDiscoverySchedule.setText(config.getActiveDiscoveryPollSchedule());
      enableActiveDiscovery(config.getDiscoveryType() == NetworkDiscoveryConfig.DISCOVERY_TYPE_ACTIVE || config.getDiscoveryType() == NetworkDiscoveryConfig.DISCOVERY_TYPE_ACTIVE_PASSIVE);
      enablePassiveDiscovery(config.getDiscoveryType() == NetworkDiscoveryConfig.DISCOVERY_TYPE_PASSIVE || config.getDiscoveryType() == NetworkDiscoveryConfig.DISCOVERY_TYPE_ACTIVE_PASSIVE);

      checkFilterRange.setSelection((config.getFilterFlags() & NetworkDiscoveryFilterFlags.CHECK_ADDRESS_RANGE) != 0);
      checkFilterScript.setSelection((config.getFilterFlags() & NetworkDiscoveryFilterFlags.EXECUTE_SCRIPT) != 0);
      checkFilterProtocols.setSelection((config.getFilterFlags() & NetworkDiscoveryFilterFlags.CHECK_PROTOCOLS) != 0);
      checkAllowAgent.setSelection((config.getFilterFlags() & NetworkDiscoveryFilterFlags.PROTOCOL_AGENT) != 0);
      checkAllowSNMP.setSelection((config.getFilterFlags() & NetworkDiscoveryFilterFlags.PROTOCOL_SNMP) != 0);
      checkAllowSSH.setSelection((config.getFilterFlags() & NetworkDiscoveryFilterFlags.PROTOCOL_SSH) != 0);

      boolean enableProtocolFilter = (config.getFilterFlags() & NetworkDiscoveryFilterFlags.CHECK_PROTOCOLS) != 0;
      checkAllowAgent.setEnabled(enableProtocolFilter);
      checkAllowSNMP.setEnabled(enableProtocolFilter);
      checkAllowSSH.setEnabled(enableProtocolFilter);

      activeDiscoveryAddressList.setInput(config.getTargets().toArray());
      filterAddressList.setInput(config.getAddressFilter().toArray());

      filterScript.setScriptName(config.getFilterScript());

      modified = false;
   }

   /**
    * Mark view as modified
    */
   private void setModified()
   {
      modified = true;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
      new Job(i18n.tr("Saving network discovery configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            config.save();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  modified = false;
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot save network discovery configuration");
         }
      }.start();
   }

   /**
    * Add element to active discovery range list
    */
   private void addTargetAddressListElement()
   {
      AddressListElementEditDialog dlg = new AddressListElementEditDialog(getWindow().getShell(), true, true, null);
      if (dlg.open() == Window.OK)
      {
         final List<InetAddressListElement> list = config.getTargets();
         InetAddressListElement element = dlg.getElement();
         if (!list.contains(element))
         {
            list.add(element);
            activeDiscoveryAddressList.setInput(list.toArray());
            setModified();
         }
      }
   }
   
   /**
    * Edit active discovery range element
    */
   private void editTargetAddressListElement()
   {
      IStructuredSelection selection = activeDiscoveryAddressList.getStructuredSelection();
      if (selection.size() != 1)
         return;

      AddressListElementEditDialog dlg = new AddressListElementEditDialog(getWindow().getShell(), true, true, (InetAddressListElement)selection.getFirstElement());
      if (dlg.open() == Window.OK)
      {
         final List<InetAddressListElement> list = config.getTargets();
         activeDiscoveryAddressList.setInput(list.toArray());
         setModified();
      }
   }

   /**
    * Remove element(s) from active discovery range list
    */
   private void removeTargetAddressListElements()
   {
      final List<InetAddressListElement> list = config.getTargets();
      IStructuredSelection selection = (IStructuredSelection)activeDiscoveryAddressList.getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            list.remove(o);
         }
         activeDiscoveryAddressList.setInput(list.toArray());
         setModified();
      }
   }
   
   /**
    * Scan select address range(s)
    */
   private void scanAddressRange()
   {
      IStructuredSelection selection = activeDiscoveryAddressList.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Active Discovery"), i18n.tr("Are you sure you want to start manual scan for selected ranges?")))
         return;

      final List<InetAddressListElement> list = new ArrayList<InetAddressListElement>();
      for(Object o : selection.toList())
      {
         list.add((InetAddressListElement)o);
      }

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Running address range scan"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.startManualActiveDiscovery(list);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  StringBuilder sb = new StringBuilder("Scan started for the following address ranges:");
                  for(InetAddressListElement e : list)
                  {
                     sb.append("\n");
                     sb.append((e.getType() == InetAddressListElement.SUBNET) ? e.getBaseAddress().getHostAddress() + "/" + e.getMaskBits() : e.getBaseAddress().getHostAddress() + " - " + e.getEndAddress().getHostAddress());
                  }
                  addMessage(MessageArea.INFORMATION, sb.toString());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot start address range scan");
         }
      }.start();
   }

   /**
    * Add element to address filter
    */
   private void addAddressFilterElement()
   {
      AddressListElementEditDialog dlg = new AddressListElementEditDialog(getWindow().getShell(), false, false, null);
      if (dlg.open() == Window.OK)
      {
         final List<InetAddressListElement> list = config.getAddressFilter();
         InetAddressListElement element = dlg.getElement();
         if (!list.contains(element))
         {
            list.add(element);
            filterAddressList.setInput(list.toArray());
            setModified();
         }
      }
   }

   /**
    * Edit address filter element
    */
   private void editAddressFilterElement()
   {
      IStructuredSelection selection = filterAddressList.getStructuredSelection();
      if (selection.size() != 1)
         return;

      AddressListElementEditDialog dlg = new AddressListElementEditDialog(getWindow().getShell(), false, false, (InetAddressListElement)selection.getFirstElement());
      if (dlg.open() == Window.OK)
      {
         final List<InetAddressListElement> list = config.getAddressFilter();
         filterAddressList.setInput(list.toArray());
         setModified();
      }
   }

   /**
    * Remove element(s) from address filter
    */
   private void removeAddressFilterElements()
   {
      final List<InetAddressListElement> list = config.getAddressFilter();
      IStructuredSelection selection = (IStructuredSelection)filterAddressList.getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            list.remove(o);
         }
         filterAddressList.setInput(list.toArray());
         setModified();
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return modified;
   }
}
