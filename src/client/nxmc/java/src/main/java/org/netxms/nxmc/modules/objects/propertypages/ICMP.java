/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.propertypages;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.IcmpStatCollectionMode;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.ComparatorHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "ICMP" property page for node
 */
public class ICMP extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(ICMP.class);

   private AbstractNode node;
   private ObjectSelector icmpProxy;
   private Button radioIcmpStatCollectionDefault;
   private Button radioIcmpStatCollectionOn;
   private Button radioIcmpStatCollectionOff;
   private TableViewer icmpTargets;
   private Set<InetAddress> icmpTargetSet = new HashSet<InetAddress>();

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public ICMP(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(ICMP.class).tr("ICMP"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "communication.icmp";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getParentId()
    */
   @Override
   public String getParentId()
   {
      return "communication";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return object instanceof AbstractNode;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      node = (AbstractNode)object;
      icmpTargetSet.addAll(Arrays.asList(node.getIcmpTargets()));

      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);
      
      icmpProxy = new ObjectSelector(dialogArea, SWT.NONE, true);
      icmpProxy.setLabel(i18n.tr("Proxy"));
      icmpProxy.setObjectId(node.getIcmpProxyId());
      icmpProxy.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      
      Group statCollectionGroup = new Group(dialogArea, SWT.NONE);
      statCollectionGroup.setText("ICMP response statistic collection");
      layout = new GridLayout();
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = true;
      statCollectionGroup.setLayout(layout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      statCollectionGroup.setLayoutData(gd);
   
      radioIcmpStatCollectionDefault = new Button(statCollectionGroup, SWT.RADIO);
      radioIcmpStatCollectionDefault.setText(i18n.tr("De&fault"));
      radioIcmpStatCollectionDefault.setSelection(node.getIcmpStatCollectionMode() == IcmpStatCollectionMode.DEFAULT);

      radioIcmpStatCollectionOn = new Button(statCollectionGroup, SWT.RADIO);
      radioIcmpStatCollectionOn.setText(i18n.tr("On"));
      radioIcmpStatCollectionOn.setSelection(node.getIcmpStatCollectionMode() == IcmpStatCollectionMode.ON);

      radioIcmpStatCollectionOff = new Button(statCollectionGroup, SWT.RADIO);
      radioIcmpStatCollectionOff.setText(i18n.tr("Off"));
      radioIcmpStatCollectionOff.setSelection(node.getIcmpStatCollectionMode() == IcmpStatCollectionMode.OFF);
      
      Group icmpTargetGroup = new Group(dialogArea, SWT.NONE);
      icmpTargetGroup.setText("Additional targets for ICMP response statistic collection");
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      icmpTargetGroup.setLayoutData(gd);

      layout = new GridLayout();
      icmpTargetGroup.setLayout(layout);
      
      icmpTargets = new TableViewer(icmpTargetGroup, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      icmpTargets.setContentProvider(new ArrayContentProvider());
      icmpTargets.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((InetAddress)element).getHostAddress();
         }
      });
      icmpTargets.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ComparatorHelper.compareInetAddresses((InetAddress)e1, (InetAddress)e2);
         }
      });
      icmpTargets.setInput(icmpTargetSet.toArray());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      icmpTargets.getControl().setLayoutData(gd);
      
      Composite buttons = new Composite(icmpTargetGroup, SWT.NONE);
      FillLayout buttonsLayout = new FillLayout();
      buttonsLayout.spacing = WidgetHelper.INNER_SPACING;
      buttons.setLayout(buttonsLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.widthHint = 184;
      buttons.setLayoutData(gd);
      
      final Button addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(i18n.tr("Add..."));
      addButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            InputDialog dlg = new InputDialog(getShell(), i18n.tr("Add IP Address"), i18n.tr("IP address"), "", null);
            if (dlg.open() == Window.OK)
            {
               try
               {
                  InetAddress addr = InetAddress.getByName(dlg.getValue().trim());
                  icmpTargetSet.add(addr);
                  icmpTargets.setInput(icmpTargetSet.toArray());
               }
               catch(UnknownHostException ex)
               {
                  MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("IP address is invalid"));
               }
            }
         }
      });

      final Button deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("Delete"));
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            IStructuredSelection selection = (IStructuredSelection)icmpTargets.getSelection();
            for(Object addr : selection.toList())
            {
               icmpTargetSet.remove(addr);
            }
            icmpTargets.setInput(icmpTargetSet.toArray());
         }
      });
      
      icmpTargets.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            deleteButton.setEnabled(!icmpTargets.getSelection().isEmpty());
         }
      });

      return dialogArea;
   }

   /**
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   protected boolean applyChanges(final boolean isApply)
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(node.getObjectId());
      
      if (isApply)
         setValid(false);
      
      md.setIcmpProxy(icmpProxy.getObjectId());
      md.setIcmpTargets(icmpTargetSet);
      
      IcmpStatCollectionMode mode;
      if (radioIcmpStatCollectionOff.getSelection())
         mode = IcmpStatCollectionMode.OFF;
      else if (radioIcmpStatCollectionOn.getSelection())
         mode = IcmpStatCollectionMode.ON;
      else
         mode = IcmpStatCollectionMode.DEFAULT;
      md.setIcmpStatCollectionMode(mode);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating ICMP settings for node {0}", node.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot update ICMP settings for node %s"), node.getObjectName());
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> ICMP.this.setValid(true));
         }
      }.start();
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      super.performDefaults();
      icmpProxy.setObjectId(0);
   }
}
