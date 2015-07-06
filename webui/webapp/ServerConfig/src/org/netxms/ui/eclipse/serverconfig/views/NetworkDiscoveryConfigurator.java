/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.ISaveablePart;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.IpAddressListElement;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.NetworkDiscovery;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptSelector;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.serverconfig.dialogs.AddAddressListElementDialog;
import org.netxms.ui.eclipse.serverconfig.views.helpers.AddressListElementComparator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.DiscoveryConfig;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Configurator for network discovery
 */
public class NetworkDiscoveryConfigurator extends ViewPart implements ISaveablePart
{
   public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.NetworkDiscoveryConfigurator"; //$NON-NLS-1$

   NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   private DiscoveryConfig config;
   private boolean modified = false;
   private FormToolkit toolkit;
   private ScrolledForm form;
   private Button radioDiscoveryOff;
   private Button radioDiscoveryPassive;
   private Button radioDiscoveryActive;
   private Button checkUseSnmpTraps;
   private Button radioFilterOff;
   private Button radioFilterCustom;
   private Button radioFilterAuto;
   private ScriptSelector filterScript;
   private Button checkAgentOnly;
   private Button checkSnmpOnly;
   private Button checkRangeOnly;
   private TableViewer filterAddressList;
   private TableViewer activeDiscoveryAddressList;
   private Action actionSave;

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
    */
   @Override
   public void init(IViewSite site, IMemento memento) throws PartInitException
   {
      super.init(site, memento);
      if (memento != null)
      {
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
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets .Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      toolkit = new FormToolkit(getSite().getShell().getDisplay());
      form = toolkit.createScrolledForm(parent);
      form.setText(Messages.get().NetworkDiscoveryConfigurator_FormTitle);

      TableWrapLayout layout = new TableWrapLayout();
      layout.numColumns = 2;
      form.getBody().setLayout(layout);

      createGeneralSection();
      createFilterSection();
      createActiveDiscoverySection();
      createSubnetFilterSection();

      createActions();
      contributeToActionBars();
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
      Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
      section.setText(Messages.get().NetworkDiscoveryConfigurator_SectionGeneral);
      section.setDescription(Messages.get().NetworkDiscoveryConfigurator_SectionGeneralDescr);
      TableWrapData td = new TableWrapData();
      td.align = TableWrapData.FILL;
      td.grabHorizontal = true;
      section.setLayoutData(td);

      Composite clientArea = toolkit.createComposite(section);
      GridLayout layout = new GridLayout();
      clientArea.setLayout(layout);
      section.setClient(clientArea);

      final SelectionListener listener = new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            setModified();
            if (radioDiscoveryOff.getSelection())
            {
               config.setEnabled(false);
            }
            else
            {
               config.setEnabled(true);
               config.setActive(radioDiscoveryActive.getSelection());
            }
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      };

      radioDiscoveryOff = toolkit.createButton(clientArea, Messages.get().NetworkDiscoveryConfigurator_Disabled, SWT.RADIO);
      radioDiscoveryOff.addSelectionListener(listener);
      radioDiscoveryPassive = toolkit.createButton(clientArea, Messages.get().NetworkDiscoveryConfigurator_PassiveDiscovery,
            SWT.RADIO);
      radioDiscoveryPassive.addSelectionListener(listener);
      radioDiscoveryActive = toolkit.createButton(clientArea, Messages.get().NetworkDiscoveryConfigurator_ActiveDiscovery,
            SWT.RADIO);
      radioDiscoveryActive.addSelectionListener(listener);

      checkUseSnmpTraps = toolkit.createButton(clientArea, Messages.get().NetworkDiscoveryConfigurator_UseSNMPTrapsForDiscovery,
            SWT.CHECK);
      checkUseSnmpTraps.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            config.setUseSnmpTraps(checkUseSnmpTraps.getSelection());
            setModified();
         }
      });
      GridData gd = new GridData();
      gd.verticalIndent = 10;
      checkUseSnmpTraps.setLayoutData(gd);
   }

   /**
    * Create "Filter" section
    */
   private void createFilterSection()
   {
      Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
      section.setText(Messages.get().NetworkDiscoveryConfigurator_SectionFilter);
      section.setDescription(Messages.get().NetworkDiscoveryConfigurator_SectionFilterDescr);
      TableWrapData td = new TableWrapData();
      td.align = TableWrapData.FILL;
      td.grabHorizontal = true;
      section.setLayoutData(td);

      Composite clientArea = toolkit.createComposite(section);
      GridLayout layout = new GridLayout();
      clientArea.setLayout(layout);
      section.setClient(clientArea);

      final SelectionListener radioButtonListener = new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            updateElementEnablement();
            setModified();
            if (radioFilterOff.getSelection())
            {
               config.setFilter(NetworkDiscovery.FILTER_NONE);
            }
            else if (radioFilterAuto.getSelection())
            {
               config.setFilter(NetworkDiscovery.FILTER_AUTO);
            }
            else
            {
               config.setFilter(filterScript.getScriptName());
            }
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      };

      final SelectionListener checkBoxListener = new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            setModified();
            int flags = 0;
            if (checkAgentOnly.getSelection())
               flags |= NetworkDiscovery.FILTER_ALLOW_AGENT;
            if (checkSnmpOnly.getSelection())
               flags |= NetworkDiscovery.FILTER_ALLOW_SNMP;
            if (checkRangeOnly.getSelection())
               flags |= NetworkDiscovery.FILTER_LIMIT_BY_RANGE;
            config.setFilterFlags(flags);
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      };

      radioFilterOff = toolkit.createButton(clientArea, Messages.get().NetworkDiscoveryConfigurator_NoFiltering, SWT.RADIO);
      radioFilterOff.addSelectionListener(radioButtonListener);
      radioFilterCustom = toolkit.createButton(clientArea, Messages.get().NetworkDiscoveryConfigurator_CustomScript, SWT.RADIO);
      radioFilterCustom.addSelectionListener(radioButtonListener);
      filterScript = new ScriptSelector(clientArea, SWT.NONE, true, false);
      toolkit.adapt(filterScript);
      filterScript.getTextControl().setBackground(clientArea.getBackground());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalIndent = 20;
      filterScript.setLayoutData(gd);
      filterScript.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            if (radioFilterCustom.getSelection())
            {
               setModified();
               config.setFilter(filterScript.getScriptName());
            }
         }
      });

      radioFilterAuto = toolkit.createButton(clientArea, Messages.get().NetworkDiscoveryConfigurator_AutoScript, SWT.RADIO);
      radioFilterAuto.addSelectionListener(radioButtonListener);

      checkAgentOnly = toolkit.createButton(clientArea, Messages.get().NetworkDiscoveryConfigurator_AcceptAgent, SWT.CHECK);
      checkAgentOnly.addSelectionListener(checkBoxListener);
      gd = new GridData();
      gd.horizontalIndent = 20;
      checkAgentOnly.setLayoutData(gd);

      checkSnmpOnly = toolkit.createButton(clientArea, Messages.get().NetworkDiscoveryConfigurator_AcceptSNMP, SWT.CHECK);
      checkSnmpOnly.addSelectionListener(checkBoxListener);
      gd = new GridData();
      gd.horizontalIndent = 20;
      checkSnmpOnly.setLayoutData(gd);

      checkRangeOnly = toolkit.createButton(clientArea, Messages.get().NetworkDiscoveryConfigurator_AcceptRange, SWT.CHECK);
      checkRangeOnly.addSelectionListener(checkBoxListener);
      gd = new GridData();
      gd.horizontalIndent = 20;
      checkRangeOnly.setLayoutData(gd);
   }

   /**
    * Create "Active Discovery Targets" section
    */
   private void createActiveDiscoverySection()
   {
      Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
      section.setText(Messages.get().NetworkDiscoveryConfigurator_SectionActiveDiscoveryTargets);
      section.setDescription(Messages.get().NetworkDiscoveryConfigurator_SectionActiveDiscoveryTargetsDescr);
      TableWrapData td = new TableWrapData();
      td.align = TableWrapData.FILL;
      td.grabHorizontal = true;
      section.setLayoutData(td);

      Composite clientArea = toolkit.createComposite(section);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      section.setClient(clientArea);

      activeDiscoveryAddressList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      toolkit.adapt(activeDiscoveryAddressList.getTable());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 2;
      gd.heightHint = 100;
      activeDiscoveryAddressList.getTable().setLayoutData(gd);
      activeDiscoveryAddressList.getTable().setSortDirection(SWT.UP);
      activeDiscoveryAddressList.setContentProvider(new ArrayContentProvider());
      activeDiscoveryAddressList.setComparator(new AddressListElementComparator());

      final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(Messages.get().NetworkDiscoveryConfigurator_Add);
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
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

      final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(Messages.get().NetworkDiscoveryConfigurator_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
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
   }

   /**
    * Create "Address Filters" section
    */
   private void createSubnetFilterSection()
   {
      Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
      section.setText(Messages.get().NetworkDiscoveryConfigurator_SectionAddressFilters);
      section.setDescription(Messages.get().NetworkDiscoveryConfigurator_SectionAddressFiltersDescr);
      TableWrapData td = new TableWrapData();
      td.align = TableWrapData.FILL;
      td.grabHorizontal = true;
      section.setLayoutData(td);

      Composite clientArea = toolkit.createComposite(section);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      section.setClient(clientArea);

      filterAddressList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      toolkit.adapt(filterAddressList.getTable());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 2;
      gd.heightHint = 100;
      filterAddressList.getTable().setLayoutData(gd);
      filterAddressList.getTable().setSortDirection(SWT.UP);
      filterAddressList.setContentProvider(new ArrayContentProvider());
      filterAddressList.setComparator(new AddressListElementComparator());

      final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(Messages.get().NetworkDiscoveryConfigurator_Add);
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
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

      final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(Messages.get().NetworkDiscoveryConfigurator_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
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

   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      form.setFocus();
   }

   /**
    * @param config
    */
   public void setConfig(DiscoveryConfig config)
   {
      this.config = config;

      radioDiscoveryOff.setSelection(!config.isEnabled());
      radioDiscoveryPassive.setSelection(config.isEnabled() && !config.isActive());
      radioDiscoveryActive.setSelection(config.isEnabled() && config.isActive());
      checkUseSnmpTraps.setSelection(config.isUseSnmpTraps());

      if (config.getFilter().equalsIgnoreCase(NetworkDiscovery.FILTER_NONE) || config.getFilter().isEmpty())
      {
         radioFilterOff.setSelection(true);
      }
      else if (config.getFilter().equalsIgnoreCase(NetworkDiscovery.FILTER_AUTO))
      {
         radioFilterAuto.setSelection(true);
      }
      else
      {
         radioFilterCustom.setSelection(true);
         filterScript.setScriptName(config.getFilter());
      }

      checkAgentOnly.setSelection((config.getFilterFlags() & NetworkDiscovery.FILTER_ALLOW_AGENT) != 0);
      checkSnmpOnly.setSelection((config.getFilterFlags() & NetworkDiscovery.FILTER_ALLOW_SNMP) != 0);
      checkRangeOnly.setSelection((config.getFilterFlags() & NetworkDiscovery.FILTER_LIMIT_BY_RANGE) != 0);

      activeDiscoveryAddressList.setInput(config.getTargets().toArray());
      filterAddressList.setInput(config.getAddressFilter().toArray());

      updateElementEnablement();
      modified = false;
      firePropertyChange(PROP_DIRTY);
   }

   /**
    * Update enabled state of elements
    */
   private void updateElementEnablement()
   {
      // Filter section
      if (radioFilterOff.getSelection())
      {
         filterScript.setEnabled(false);
         checkAgentOnly.setEnabled(false);
         checkSnmpOnly.setEnabled(false);
         checkRangeOnly.setEnabled(false);
      }
      else if (radioFilterAuto.getSelection())
      {
         filterScript.setEnabled(false);
         checkAgentOnly.setEnabled(true);
         checkSnmpOnly.setEnabled(true);
         checkRangeOnly.setEnabled(true);
      }
      if (radioFilterCustom.getSelection())
      {
         filterScript.setEnabled(true);
         checkAgentOnly.setEnabled(false);
         checkSnmpOnly.setEnabled(false);
         checkRangeOnly.setEnabled(false);
      }
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

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor )
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

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#doSaveAs()
    */
   @Override
   public void doSaveAs()
   {
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#isDirty()
    */
   @Override
   public boolean isDirty()
   {
      return modified;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
    */
   @Override
   public boolean isSaveAsAllowed()
   {
      return false;
   }

   /*
    * (non-Javadoc)
    * 
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
    * Add element to address filter
    */
   private void addTargetAddressListElement()
   {
      AddAddressListElementDialog dlg = new AddAddressListElementDialog(getSite().getShell());
      if (dlg.open() == Window.OK)
      {
         final List<IpAddressListElement> list = config.getTargets();
         IpAddressListElement element = new IpAddressListElement(dlg.getType(), dlg.getAddress1(), dlg.getAddress2());
         if (!list.contains(element))
         {
            list.add(element);
            activeDiscoveryAddressList.setInput(list.toArray());
            setModified();
         }
      }
   }

   /**
    * Remove element(s) from address filter
    */
   private void removeTargetAddressListElements()
   {
      final List<IpAddressListElement> list = config.getTargets();
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
    * Add element to address filter
    */
   private void addAddressFilterElement()
   {
      AddAddressListElementDialog dlg = new AddAddressListElementDialog(getSite().getShell());
      if (dlg.open() == Window.OK)
      {
         final List<IpAddressListElement> list = config.getAddressFilter();
         IpAddressListElement element = new IpAddressListElement(dlg.getType(), dlg.getAddress1(), dlg.getAddress2());
         if (!list.contains(element))
         {
            list.add(element);
            filterAddressList.setInput(list.toArray());
            setModified();
         }
      }
   }

   /**
    * Remove element(s) from address filter
    */
   private void removeAddressFilterElements()
   {
      final List<IpAddressListElement> list = config.getAddressFilter();
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
