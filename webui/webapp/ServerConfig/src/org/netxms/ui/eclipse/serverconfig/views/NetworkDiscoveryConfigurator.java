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
package org.netxms.ui.eclipse.serverconfig.views;

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
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.ISaveablePart;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.InetAddressListElement;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.NetworkDiscoveryFilterFlags;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptSelector;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.serverconfig.dialogs.AddAddressListElementDialog;
import org.netxms.ui.eclipse.serverconfig.views.helpers.AddressListElementComparator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.AddressListLabelProvider;
import org.netxms.ui.eclipse.serverconfig.views.helpers.DiscoveryConfig;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Configurator for network discovery
 */
public class NetworkDiscoveryConfigurator extends ViewPart implements ISaveablePart
{
   public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.NetworkDiscoveryConfigurator"; //$NON-NLS-1$

   public static final int RANGE = 0;
   public static final int PROXY = 1;
   public static final int COMMENTS = 2;

   private NXCSession session = ConsoleSharedData.getSession();
   private DiscoveryConfig config;
   private boolean modified = false;
   private Composite content;
   private Button radioDiscoveryOff;
   private Button radioDiscoveryPassive;
   private Button radioDiscoveryActive;
   private Button radioDiscoveryActiveAndPassive;
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
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
    */
   @Override
   public void init(IViewSite site, IMemento memento) throws PartInitException
   {
      super.init(site, memento);         
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      ScrolledComposite scroller = new ScrolledComposite(parent, SWT.V_SCROLL);
      scroller.setExpandHorizontal(true);
      scroller.setExpandVertical(true);
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
      contributeToActionBars();

      // Restoring, load config
      new ConsoleJob(Messages.get().NetworkDiscoveryConfigurator_LoadJobName, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final DiscoveryConfig loadedConfig = DiscoveryConfig.load(session);
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
            return Messages.get().NetworkDiscoveryConfigurator_LoadJobError;
         }
      }.start();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionSave = new Action(Messages.get().NetworkDiscoveryConfigurator_Save, SharedIcons.SAVE) {
         @Override
         public void run()
         {
            save();
         }
      };
   }

   /**
    * Contribute actions to action bar
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager Menu manager for pull-down menu
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionSave);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionSave);
   }

   /**
    * Create "General" section
    */
   private void createGeneralSection()
   {
      Group section = new Group(content, SWT.NONE);
      section.setBackground(content.getBackground());
      section.setText(Messages.get().NetworkDiscoveryConfigurator_SectionGeneral);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      section.setLayoutData(gd);

      GridLayout layout = new GridLayout();
      section.setLayout(layout);

      final SelectionListener listener = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            setModified();
            if (radioDiscoveryOff.getSelection())
            {
               config.setDiscoveryType(DiscoveryConfig.DISCOVERY_TYPE_NONE);
               enableActiveDiscovery(false);
               enablePassiveDiscovery(false);
            }
            else if (radioDiscoveryPassive.getSelection())
            {
               config.setDiscoveryType(DiscoveryConfig.DISCOVERY_TYPE_PASSIVE);
               enableActiveDiscovery(false);
               enablePassiveDiscovery(true);
            }
            else if (radioDiscoveryActive.getSelection())
            {
               config.setDiscoveryType(DiscoveryConfig.DISCOVERY_TYPE_ACTIVE);
               enableActiveDiscovery(true);
               enablePassiveDiscovery(false);
            }
            else if (radioDiscoveryActiveAndPassive.getSelection())
            {
               config.setDiscoveryType(DiscoveryConfig.DISCOVERY_TYPE_ACTIVE_PASSIVE);
               enableActiveDiscovery(true);
               enablePassiveDiscovery(true);
            }
         }
      };

      radioDiscoveryOff = new Button(section, SWT.RADIO);
      radioDiscoveryOff.setText(Messages.get().NetworkDiscoveryConfigurator_Disabled);
      radioDiscoveryOff.addSelectionListener(listener);
      radioDiscoveryPassive = new Button(section, SWT.RADIO);
      radioDiscoveryPassive.setText(Messages.get().NetworkDiscoveryConfigurator_PassiveDiscovery);
      radioDiscoveryPassive.addSelectionListener(listener);
      radioDiscoveryActive = new Button(section, SWT.RADIO);
      radioDiscoveryActive.setText("Active only");
      radioDiscoveryActive.addSelectionListener(listener);
      radioDiscoveryActiveAndPassive = new Button(section, SWT.RADIO);
      radioDiscoveryActiveAndPassive.setText(Messages.get().NetworkDiscoveryConfigurator_ActiveDiscovery);
      radioDiscoveryActiveAndPassive.addSelectionListener(listener);      

      checkUseSnmpTraps = new Button(section, SWT.CHECK);
      checkUseSnmpTraps.setText(Messages.get().NetworkDiscoveryConfigurator_UseSNMPTrapsForDiscovery);
      checkUseSnmpTraps.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            config.setUseSnmpTraps(checkUseSnmpTraps.getSelection());
            setModified();
         }
      });
      gd = new GridData();
      gd.verticalIndent = 10;
      checkUseSnmpTraps.setLayoutData(gd);

      checkUseSyslog = new Button(section, SWT.CHECK);
      checkUseSyslog.setText(Messages.get().NetworkDiscoveryConfigurator_UseSyslogForDiscovery);
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
   }

   /**
    * Create "Schedule" section
    */
   private void createScheduleSection()
   {
      Group section = new Group(content, SWT.NONE);
      section.setBackground(content.getBackground());
      section.setText("Schedule");
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      section.setLayoutData(gd);

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      section.setLayout(layout);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      passiveDiscoveryInterval = WidgetHelper.createLabeledSpinner(section, SWT.BORDER, "Passive discovery interval", 0, 0xffffff, gd);
      passiveDiscoveryInterval.setBackground(section.getBackground());
      passiveDiscoveryInterval.getParent().setBackground(section.getBackground());
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
      
      activeDiscoveryScheduleLabel = new Label(section, SWT.LEFT);
      activeDiscoveryScheduleLabel.setText("Active discovery schedule configuration");
      activeDiscoveryScheduleLabel.setBackground(section.getBackground());
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
               activeDiscoveryInterval.setSelection(config.getActiveDiscoveryPollInterval() == 0 ? DiscoveryConfig.DEFAULT_ACTIVE_INTERVAL : config.getActiveDiscoveryPollInterval());
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

      radioActiveDiscoveryInterval = new Button(section, SWT.RADIO);
      radioActiveDiscoveryInterval.setText("Interval");
      radioActiveDiscoveryInterval.addSelectionListener(listener);
      gd = new GridData();
      radioActiveDiscoveryInterval.setLayoutData(gd);
      
      radioActiveDiscoverySchedule = new Button(section, SWT.RADIO);
      radioActiveDiscoverySchedule.setText("Schedule");
      radioActiveDiscoverySchedule.addSelectionListener(listener);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.LEFT;
      radioActiveDiscoverySchedule.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      activeDiscoveryInterval = WidgetHelper.createLabeledSpinner(section, SWT.BORDER, "Active discovery interval", 0, 0xffffff, gd);
      activeDiscoveryInterval.setBackground(section.getBackground());
      activeDiscoveryInterval.getParent().setBackground(section.getBackground());
      activeDiscoveryInterval.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            config.setActiveDiscoveryPollInterval(Integer.parseInt(activeDiscoveryInterval.getText()));
            setModified();
         }
      });
      
      activeDiscoverySchedule = new LabeledText(section, SWT.NONE, SWT.SINGLE | SWT.BORDER);
      activeDiscoverySchedule.setLabel("Active discovery schedule");
      activeDiscoverySchedule.setBackground(section.getBackground());
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
      Group section = new Group(content, SWT.NONE);
      section.setBackground(content.getBackground());
      section.setText(Messages.get().NetworkDiscoveryConfigurator_SectionFilter);
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalSpan = 3;
      section.setLayoutData(gd);

      GridLayout layout = new GridLayout();
      section.setLayout(layout);

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
         }
      };

      checkFilterRange = new Button(section, SWT.CHECK);
      checkFilterRange.setText("By address range");
      checkFilterRange.addSelectionListener(checkBoxListener);

      Composite addressRangeEditor = new Composite(section, SWT.NONE);
      addressRangeEditor.setBackground(section.getBackground());
      gd = new GridData();
      gd.horizontalIndent = 20;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      addressRangeEditor.setLayoutData(gd);
      GridLayout addressRangeLayout = new GridLayout();
      addressRangeLayout.numColumns = 2;
      addressRangeEditor.setLayout(addressRangeLayout);

      final String[] names = { "Range", "Comment" };
      final int[] widths = { 150, 150 };
      filterAddressList = new SortableTableViewer(addressRangeEditor, names, widths, 0, SWT.DOWN, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 3;
      gd.heightHint = 200;
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
      linkAdd.setText(Messages.get().NetworkDiscoveryConfigurator_Add);
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(section.getBackground());
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
      linkEdit.setText("Edit...");
      linkEdit.setImage(SharedIcons.IMG_EDIT);
      linkEdit.setBackground(section.getBackground());
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
      linkRemove.setText(Messages.get().NetworkDiscoveryConfigurator_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(section.getBackground());
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

      checkFilterProtocols = new Button(section, SWT.CHECK);
      checkFilterProtocols.setText("By communication protocols");
      checkFilterProtocols.addSelectionListener(checkBoxListener);

      checkAllowAgent = new Button(section, SWT.CHECK);
      checkAllowAgent.setText(Messages.get().NetworkDiscoveryConfigurator_AcceptAgent);
      checkAllowAgent.addSelectionListener(checkBoxListener);
      gd = new GridData();
      gd.horizontalIndent = 20;
      checkAllowAgent.setLayoutData(gd);

      checkAllowSNMP = new Button(section, SWT.CHECK);
      checkAllowSNMP.setText(Messages.get().NetworkDiscoveryConfigurator_AcceptSNMP);
      checkAllowSNMP.addSelectionListener(checkBoxListener);
      gd = new GridData();
      gd.horizontalIndent = 20;
      checkAllowSNMP.setLayoutData(gd);

      checkAllowSSH = new Button(section, SWT.CHECK);
      checkAllowSSH.setText("Accept node if it is accessible via SS&H");
      checkAllowSSH.addSelectionListener(checkBoxListener);
      gd = new GridData();
      gd.horizontalIndent = 20;
      checkAllowSSH.setLayoutData(gd);

      checkFilterScript = new Button(section, SWT.CHECK);
      checkFilterScript.setText("With custom script");
      checkFilterScript.addSelectionListener(checkBoxListener);

      filterScript = new ScriptSelector(section, SWT.NONE, true, false);
      filterScript.setBackground(section.getBackground());
      filterScript.getTextControl().setBackground(section.getBackground());
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
      Group section = new Group(content, SWT.NONE);
      section.setBackground(content.getBackground());
      section.setText(Messages.get().NetworkDiscoveryConfigurator_SectionActiveDiscoveryTargets);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      section.setLayoutData(gd);

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      section.setLayout(layout);

      final String[] names = { "Range", "Proxy", "Comments" };
      final int[] widths = { 150, 150, 150 };
      activeDiscoveryAddressList = new SortableTableViewer(section, names, widths, 0, SWT.DOWN, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 4;
      gd.heightHint = 200;
      activeDiscoveryAddressList.getTable().setLayoutData(gd);
      activeDiscoveryAddressList.getTable().setSortDirection(SWT.UP);
      activeDiscoveryAddressList.setContentProvider(new ArrayContentProvider());
      activeDiscoveryAddressList.setLabelProvider(new AddressListLabelProvider(true));
      activeDiscoveryAddressList.setComparator(new AddressListElementComparator(true));
      activeDiscoveryAddressList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editTargetAddressListElement();
         }
      });

      final ImageHyperlink linkAdd = new ImageHyperlink(section, SWT.NONE);
      linkAdd.setText(Messages.get().NetworkDiscoveryConfigurator_Add);
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(section.getBackground());
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

      final ImageHyperlink linkEdit = new ImageHyperlink(section, SWT.NONE);
      linkEdit.setText("Edit...");
      linkEdit.setImage(SharedIcons.IMG_EDIT);
      linkEdit.setBackground(section.getBackground());
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

      final ImageHyperlink linkRemove = new ImageHyperlink(section, SWT.NONE);
      linkRemove.setText(Messages.get().NetworkDiscoveryConfigurator_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(section.getBackground());
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

      final ImageHyperlink runActiveDiscovery = new ImageHyperlink(section, SWT.NONE);
      runActiveDiscovery.setText("Scan");
      runActiveDiscovery.setToolTipText("Runs active discovery on selected ranges");
      runActiveDiscovery.setImage(SharedIcons.IMG_EXECUTE);
      runActiveDiscovery.setBackground(section.getBackground());
      runActiveDiscovery.setEnabled(false);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      runActiveDiscovery.setLayoutData(gd);
      runActiveDiscovery.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            runManualActiveDiscovery();
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
   public void setConfig(DiscoveryConfig config)
   {
      this.config = config;

      radioDiscoveryOff.setSelection(config.getDiscoveryType() == DiscoveryConfig.DISCOVERY_TYPE_NONE);
      radioDiscoveryPassive.setSelection(config.getDiscoveryType() == DiscoveryConfig.DISCOVERY_TYPE_PASSIVE);
      radioDiscoveryActive.setSelection(config.getDiscoveryType() == DiscoveryConfig.DISCOVERY_TYPE_ACTIVE);
      radioDiscoveryActiveAndPassive.setSelection(config.getDiscoveryType() == DiscoveryConfig.DISCOVERY_TYPE_ACTIVE_PASSIVE);
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
      enableActiveDiscovery(config.getDiscoveryType() == DiscoveryConfig.DISCOVERY_TYPE_ACTIVE || config.getDiscoveryType() == DiscoveryConfig.DISCOVERY_TYPE_ACTIVE_PASSIVE);
      enablePassiveDiscovery(config.getDiscoveryType() == DiscoveryConfig.DISCOVERY_TYPE_PASSIVE || config.getDiscoveryType() == DiscoveryConfig.DISCOVERY_TYPE_ACTIVE_PASSIVE);

      checkFilterRange.setSelection((config.getFilterFlags() & NetworkDiscoveryFilterFlags.CHECK_ADDRESS_RANGE) != 0);
      checkFilterScript.setSelection((config.getFilterFlags() & NetworkDiscoveryFilterFlags.EXECUTE_SCRIPT) != 0);
      checkFilterProtocols.setSelection((config.getFilterFlags() & NetworkDiscoveryFilterFlags.CHECK_PROTOCOLS) != 0);
      checkAllowAgent.setSelection((config.getFilterFlags() & NetworkDiscoveryFilterFlags.PROTOCOL_AGENT) != 0);
      checkAllowSNMP.setSelection((config.getFilterFlags() & NetworkDiscoveryFilterFlags.PROTOCOL_SNMP) != 0);
      checkAllowSSH.setSelection((config.getFilterFlags() & NetworkDiscoveryFilterFlags.PROTOCOL_SSH) != 0);

      activeDiscoveryAddressList.setInput(config.getTargets().toArray());
      filterAddressList.setInput(config.getAddressFilter().toArray());

      filterScript.setScriptName(config.getFilterScript());

      modified = false;
      firePropertyChange(PROP_DIRTY);
   }

   /**
    * Mark view as modified
    */
   private void setModified()
   {
      if (!modified)
      {
         modified = true;
         firePropertyChange(PROP_DIRTY);
      }
   }

   /**
    * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
    */
   @Override
   public void doSave(IProgressMonitor monitor)
   {
      try
      {
         config.save(session);
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getSite().getShell(), Messages.get().NetworkDiscoveryConfigurator_Error,
               String.format(Messages.get().NetworkDiscoveryConfigurator_SaveErrorText, e.getLocalizedMessage()));
      }
   }

   /**
    * @see org.eclipse.ui.ISaveablePart#doSaveAs()
    */
   @Override
   public void doSaveAs()
   {
   }

   /**
    * @see org.eclipse.ui.ISaveablePart#isDirty()
    */
   @Override
   public boolean isDirty()
   {
      return modified;
   }

   /**
    * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
    */
   @Override
   public boolean isSaveAsAllowed()
   {
      return false;
   }

   /**
    * @see org.eclipse.ui.ISaveablePart#isSaveOnCloseNeeded()
    */
   @Override
   public boolean isSaveOnCloseNeeded()
   {
      return modified;
   }

   /**
    * Save settings
    */
   private void save()
   {
      new ConsoleJob(Messages.get().NetworkDiscoveryConfigurator_SaveJobName, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            config.save(session);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  modified = false;
                  firePropertyChange(PROP_DIRTY);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().NetworkDiscoveryConfigurator_SaveJobError;
         }
      }.start();
   }

   /**
    * Add element to active discovery range list
    */
   private void addTargetAddressListElement()
   {
      AddAddressListElementDialog dlg = new AddAddressListElementDialog(getSite().getShell(), true, null);
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
      IStructuredSelection selection = (IStructuredSelection)activeDiscoveryAddressList.getSelection();
      if (selection.size() != 1)
         return;
      
      AddAddressListElementDialog dlg = new AddAddressListElementDialog(getSite().getShell(), true, (InetAddressListElement)selection.getFirstElement());
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
   
   private void runManualActiveDiscovery()
   {
      final IStructuredSelection selection = (IStructuredSelection)activeDiscoveryAddressList.getSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getSite().getShell(), "Active discovery", "Are you sure you want to start manual scan for selected ranges?"))
         return;

      final List<InetAddressListElement> list = new ArrayList<InetAddressListElement>();
      for(Object o : selection.toList())
      {
         list.add((InetAddressListElement)o);
      }

      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Run active discovery poll", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.startManualActiveDiscovery(list);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  StringBuilder sb = new StringBuilder("Active discovery started for ranges:\n");
                  for (int i = 0; i < list.size(); i++)
                  {
                     InetAddressListElement e = list.get(i);
                     sb.append((e.getType() == InetAddressListElement.SUBNET) ? e.getBaseAddress().getHostAddress() + "/" + e.getMaskBits() : e.getBaseAddress().getHostAddress() + " - " + e.getEndAddress().getHostAddress());
                     if ((i+1) != list.size())
                        sb.append("\n");
                  }
                  
                  MessageDialogHelper.openInformation(getSite().getShell(), "Active discovery started", sb.toString());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Error startin active discovery";
         }
      }.start();
   }

   /**
    * Add element to address filter
    */
   private void addAddressFilterElement()
   {
      AddAddressListElementDialog dlg = new AddAddressListElementDialog(getSite().getShell(), false, null);
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
      IStructuredSelection selection = (IStructuredSelection)filterAddressList.getSelection();
      if (selection.size() != 1)
         return;
      
      AddAddressListElementDialog dlg = new AddAddressListElementDialog(getSite().getShell(), false, (InetAddressListElement)selection.getFirstElement());
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
}
