/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BaseBusinessService;
import org.netxms.client.objects.Circuit;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.DashboardTemplate;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.client.objects.interfaces.AutoBindObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Auto Bind" property page
 */
public class AutoBind extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(AutoBind.class);

   private AutoBindObject autoBindObject;
	private Button checkboxEnableBind;
	private Button checkboxEnableUnbind;
	private ScriptEditor filterSource;
   private boolean initialBind;
   private boolean initialUnbind;
	private String initialAutoBindFilter;

   /**
    * Create "auto bind" property page for given object
    *
    * @param object object to create property page for
    */
   public AutoBind(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(AutoBind.class).tr("Automatic Bind Rules"), object);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = new Composite(parent, SWT.NONE);

		autoBindObject = (AutoBindObject)object;
		if (autoBindObject == null)	// Paranoid check
			return dialogArea;

      initialBind = autoBindObject.isAutoBindEnabled();
      initialUnbind = autoBindObject.isAutoUnbindEnabled();
		initialAutoBindFilter = autoBindObject.getAutoBindFilter();

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      // Enable/disable check box
      checkboxEnableBind = new Button(dialogArea, SWT.CHECK);
      if (autoBindObject instanceof Cluster)
         checkboxEnableBind.setText(i18n.tr("Automatically add nodes selected by filter to this cluster"));
      else if (autoBindObject instanceof Circuit)
         checkboxEnableBind.setText(i18n.tr("Automatically add interfaces selected by filter to this circuit"));
      else if (autoBindObject instanceof Container || autoBindObject instanceof Collector)
         checkboxEnableBind.setText(i18n.tr("Automatically bind objects selected by filter to this container"));
      else if (autoBindObject instanceof Dashboard)
         checkboxEnableBind.setText(i18n.tr("Automatically add this dashboard to objects selected by filter"));
      else if (autoBindObject instanceof DashboardTemplate)
         checkboxEnableBind.setText(i18n.tr("Automatically create instance dashboard for objects selected by filter"));
      else if (autoBindObject instanceof NetworkMap)
         checkboxEnableBind.setText(i18n.tr("Automatically add this network map to objects selected by filter"));
      checkboxEnableBind.setSelection(autoBindObject.isAutoBindEnabled());
      checkboxEnableBind.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				if (checkboxEnableBind.getSelection())
				{
					filterSource.setEnabled(true);
					filterSource.setFocus();
					checkboxEnableUnbind.setEnabled(true);
				}
				else
				{
					filterSource.setEnabled(false);
					checkboxEnableUnbind.setEnabled(false);
				}
			}
      });

      checkboxEnableUnbind = new Button(dialogArea, SWT.CHECK);
      if (autoBindObject instanceof Cluster)
         checkboxEnableUnbind.setText(i18n.tr("Automatically remove nodes from this cluster when they no longer passes filter"));
      else if (autoBindObject instanceof Circuit)
         checkboxEnableUnbind.setText(i18n.tr("Automatically remove interfaces from this circuit when they no longer passes filter"));
      else if (autoBindObject instanceof Container || autoBindObject instanceof Collector)
         checkboxEnableUnbind.setText(i18n.tr("Automatically unbind objects from this container when they no longer passes filter"));
      else if (autoBindObject instanceof Dashboard)
         checkboxEnableUnbind.setText(i18n.tr("Automatically remove this dashboard from objects when they no longer passes filter"));
      else if (autoBindObject instanceof DashboardTemplate)
         checkboxEnableUnbind.setText(i18n.tr("Automatically delete instance dashboard when object no longer passes filter"));
      else if (autoBindObject instanceof NetworkMap)
         checkboxEnableUnbind.setText(i18n.tr("Automatically remove this network map from objects when they no longer passes filter"));
      checkboxEnableUnbind.setSelection(autoBindObject.isAutoUnbindEnabled());
      checkboxEnableUnbind.setEnabled(autoBindObject.isAutoBindEnabled());

      // Filtering script
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Filtering script"));

      GridData gd = new GridData();
      gd.verticalIndent = WidgetHelper.DIALOG_SPACING;
      label.setLayoutData(gd);

      String hints;
      if (autoBindObject instanceof Cluster)
         hints = i18n.tr("Variables:\n\t$node\tnode being tested (null if object is not a node).\n\t$object\tobject being tested.\n\t$cluster\tthis cluster object.\n\nReturn value: true to add node to this cluster, false to remove, null to make no changes.");
      else if (autoBindObject instanceof Container || autoBindObject instanceof Collector)
         hints = i18n.tr("Variables:\n\t$node\tnode being tested (null if object is not a node).\n\t$object\tobject being tested.\n\t$container\tthis container object.\n\nReturn value: true to bind node to this container, false to unbind, null to make no changes.");
      else if (autoBindObject instanceof Circuit)
         hints = i18n.tr("Variables:\n\t$object\tinterface being tested.\n\t$container\tthis container object.\n\nReturn value: true to bind interface to this container, false to unbind, null to make no changes.");
      else if (autoBindObject instanceof Dashboard)
         hints = i18n.tr("Variables:\n\t$node\tnode being tested (null if object is not a node).\n\t$object\tobject being tested.\n\t$dashboard\tthis dashboard object.\n\nReturn value: true to add this dashboard to an object, false to remove, null to make no changes.");
      else if (autoBindObject instanceof DashboardTemplate)
         hints = i18n.tr("Variables:\n\t$node\tnode being tested (null if object is not a node).\n\t$object\tobject being tested.\n\t$template\tthis dashboard template object.\n\nReturn value: true to create dashboard instance for this object, false to delete, null to make no changes."); 
      else if (autoBindObject instanceof NetworkMap)
         hints = i18n.tr("Variables:\n\t$node\tnode being tested (null if object is not a node).\n\t$object\tobject being tested.\n\t$map\tthis network map object.\n\nReturn value: true to add this network map to an object, false to remove, null to make no changes.");
      else
         hints = "";

      filterSource = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, hints);
      filterSource.setText(autoBindObject.getAutoBindFilter());
      filterSource.setEnabled(autoBindObject.isAutoBindEnabled());

		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.widthHint = 0;
      gd.heightHint = 0;
		filterSource.setLayoutData(gd);

		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
      boolean apply = checkboxEnableBind.getSelection();
      boolean remove = checkboxEnableUnbind.getSelection();

		if ((apply == initialBind) && (remove == initialUnbind) && initialAutoBindFilter.equals(filterSource.getText()))
			return true;		// Nothing to apply

		if (isApply)
			setValid(false);

		final NXCSession session =  Registry.getSession();
		final NXCObjectModificationData md = new NXCObjectModificationData(((GenericObject)autoBindObject).getObjectId());
		md.setAutoBindFilter(filterSource.getText());
      int flags = autoBindObject.getAutoBindFlags();
      flags = apply ? flags | AutoBindObject.OBJECT_BIND_FLAG : flags & ~AutoBindObject.OBJECT_BIND_FLAG;  
      flags = remove ? flags | AutoBindObject.OBJECT_UNBIND_FLAG : flags & ~AutoBindObject.OBJECT_UNBIND_FLAG;  
      md.setAutoBindFlags(flags);

      new Job(i18n.tr("Update auto-bind filter"), null, messageArea) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
		      initialBind = apply;
		      initialUnbind = remove;
				initialAutoBindFilter = md.getAutoBindFilter();
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> AutoBind.this.setValid(true));
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot change container automatic bind options");
			}
		}.start();
		
		return true;
	}

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "autoBind";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof AutoBindObject) && !(object instanceof BaseBusinessService) &&
            !(object instanceof Dashboard && ((Dashboard)object).isTemplateInstance());
   }
}
