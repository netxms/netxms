/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.networkmaps.propertypages;

import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.List;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.networkmaps.views.helpers.LinkEditor;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptSelector;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Color" property page for map link
 */
public class LinkColor extends LinkPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(LinkColor.class);

	private Button radioColorDefault;
   private Button radioColorInterface;
	private Button radioColorObject;
   private Button radioColorScript;
   private Button radioColorUtilization;
	private Button radioColorCustom;
   private Button checkUseThresholds;
   private Button checkUseUtilization;
   private ScriptSelector script;
	private ColorSelector color;
	private List list;
	private Button add;
	private Button remove;

   /**
    * Create new page.
    *
    * @param linkEditor link to edit
    */
   public LinkColor(LinkEditor linkEditor)
   {
      super(linkEditor, LocalizationHelper.getI18n(LinkColor.class).tr("Color"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = (Composite)super.createContents(parent);

      final SelectionListener listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				color.setEnabled(radioColorCustom.getSelection());
            script.setEnabled(radioColorScript.getSelection());
				list.setEnabled(radioColorObject.getSelection()); 
				add.setEnabled(radioColorObject.getSelection());
				remove.setEnabled(radioColorObject.getSelection());
				checkUseThresholds.setEnabled(radioColorObject.getSelection());
            checkUseUtilization.setEnabled(radioColorObject.getSelection());
			}
		};

      radioColorDefault = new Button(dialogArea, SWT.RADIO);
      radioColorDefault.setText(i18n.tr("&Default color"));
      radioColorDefault.setSelection(linkEditor.getColorSource() == NetworkMapLink.COLOR_SOURCE_DEFAULT);
		radioColorDefault.addSelectionListener(listener);

      radioColorInterface = new Button(dialogArea, SWT.RADIO);
      radioColorInterface.setText(i18n.tr("Based on interface &status"));
      radioColorInterface.setSelection(linkEditor.getColorSource() == NetworkMapLink.COLOR_SOURCE_INTERFACE_STATUS);
      radioColorInterface.addSelectionListener(listener);

      radioColorUtilization = new Button(dialogArea, SWT.RADIO);
      radioColorUtilization.setText(i18n.tr("Based on &interface utilization"));
      radioColorUtilization.setSelection(linkEditor.getColorSource() == NetworkMapLink.COLOR_SOURCE_LINK_UTILIZATION);
      radioColorUtilization.addSelectionListener(listener);

      radioColorObject = new Button(dialogArea, SWT.RADIO);
      radioColorObject.setText(i18n.tr("Based on &object status"));
      radioColorObject.setSelection(linkEditor.getColorSource() == NetworkMapLink.COLOR_SOURCE_OBJECT_STATUS);
		radioColorObject.addSelectionListener(listener);

      final Composite nodeSelectionGroup = new Composite(dialogArea, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      nodeSelectionGroup.setLayout(layout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      nodeSelectionGroup.setLayoutData(gd);

		list = new List(nodeSelectionGroup, SWT.BORDER | SWT.SINGLE | SWT.V_SCROLL | SWT.H_SCROLL );
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalSpan = 2;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalIndent = 20;
      list.setLayoutData(gd);
      if (linkEditor.getStatusObjects() != null)
		{
         for(Long id : linkEditor.getStatusObjects())
            list.add(getObjectName(id));
		}
      list.setEnabled(radioColorObject.getSelection());

      add = new Button(nodeSelectionGroup, SWT.PUSH);
      add.setText(i18n.tr("&Add..."));
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.verticalAlignment = SWT.TOP;
      add.setLayoutData(gd);
      add.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addObject();
         }
      });
      add.setEnabled(radioColorObject.getSelection());

      remove = new Button(nodeSelectionGroup, SWT.PUSH);
      remove.setText(i18n.tr("&Delete"));
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.verticalAlignment = SWT.TOP;
      remove.setLayoutData(gd);
      remove.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            removeObject();
         }
      });
      remove.setEnabled(radioColorObject.getSelection());

      checkUseThresholds = new Button(nodeSelectionGroup, SWT.CHECK);
      checkUseThresholds.setText("Include active &thresholds into calculation");
      checkUseThresholds.setEnabled(radioColorObject.getSelection());
      checkUseThresholds.setSelection(linkEditor.isUseActiveThresholds());
      gd = new GridData();
      gd.horizontalIndent = 17;
      gd.horizontalSpan = layout.numColumns;
      checkUseThresholds.setLayoutData(gd);

      checkUseUtilization = new Button(nodeSelectionGroup, SWT.CHECK);
      checkUseUtilization.setText("Include interface &utilization into calculation");
      checkUseUtilization.setEnabled(radioColorObject.getSelection());
      checkUseUtilization.setSelection(linkEditor.isUseInterfaceUtilization());
      gd = new GridData();
      gd.horizontalIndent = 17;
      gd.horizontalSpan = layout.numColumns;
      checkUseUtilization.setLayoutData(gd);

      radioColorScript = new Button(dialogArea, SWT.RADIO);
      radioColorScript.setText("Script");
      radioColorScript.setSelection(linkEditor.getColorSource() == NetworkMapLink.COLOR_SOURCE_SCRIPT);
      radioColorScript.addSelectionListener(listener);

      script = new ScriptSelector(dialogArea, SWT.NONE, false, false);
      script.setScriptName(linkEditor.getColorProvider());
      if (radioColorScript.getSelection())
         script.setScriptName(linkEditor.getColorProvider());
      else
         script.setEnabled(false);
      gd = new GridData();
      gd.horizontalIndent = 20;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      script.setLayoutData(gd);

      radioColorCustom = new Button(dialogArea, SWT.RADIO);
      radioColorCustom.setText(i18n.tr("&Custom color"));
      radioColorCustom.setSelection(linkEditor.getColorSource() == NetworkMapLink.COLOR_SOURCE_CUSTOM_COLOR);
		radioColorCustom.addSelectionListener(listener);

      color = new ColorSelector(dialogArea);
		if (radioColorCustom.getSelection())
			color.setColorValue(ColorConverter.rgbFromInt(linkEditor.getColor()));
		else
			color.setEnabled(false);
		gd = new GridData();
		gd.horizontalIndent = 20;
		color.getButton().setLayoutData(gd);

		return dialogArea;
	}

	/**
	 * Get objectName or parentNodeName/objectName
	 * @param objectId object id
	 * @return formatted name
	 */
	private String getObjectName(long objectId)
	{
      NXCSession session = Registry.getSession();
      AbstractObject object = session.findObjectById(objectId);
      if (object == null)
         return "[" + Long.toString(objectId) + "]";
      java.util.List<AbstractObject> parentNode = object.getParentChain(new int [] { AbstractObject.OBJECT_NODE });
      return (parentNode.size() > 0 ? parentNode.get(0).getObjectName() + " / " : "") +  object.getObjectName();	   
	}

   /**
    * Add object to status source list
    */
   private void addObject()
   {
      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell());
      dlg.enableMultiSelection(false);
      if (dlg.open() == Window.OK)
      {
         AbstractObject[] objects = dlg.getSelectedObjects(AbstractObject.class);
         if (objects.length > 0)
         {
            for(AbstractObject obj : objects)
            {
               linkEditor.addStatusObject(obj.getObjectId());
               list.add(getObjectName(obj.getObjectId())); 
            }
         }
      }
   }

   /**
    * Remove object from status source list
    */
   private void removeObject()
   {
      int index = list.getSelectionIndex();
      list.remove(index);
      linkEditor.removeStatusObjectByIndex(index);
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
		if (radioColorCustom.getSelection())
		{
         linkEditor.setColorSource(NetworkMapLink.COLOR_SOURCE_CUSTOM_COLOR);
         linkEditor.setColor(ColorConverter.rgbToInt(color.getColorValue()));
		}
      else if (radioColorInterface.getSelection())
      {
         linkEditor.setColorSource(NetworkMapLink.COLOR_SOURCE_INTERFACE_STATUS);
      }
		else if (radioColorObject.getSelection())
		{
         linkEditor.setColorSource(NetworkMapLink.COLOR_SOURCE_OBJECT_STATUS);
		}
      else if (radioColorUtilization.getSelection())
      {
         linkEditor.setColorSource(NetworkMapLink.COLOR_SOURCE_LINK_UTILIZATION);
      }
      else if (radioColorScript.getSelection())
      {
         linkEditor.setColorSource(NetworkMapLink.COLOR_SOURCE_SCRIPT);
         linkEditor.setColorProvider(script.getScriptName());
      }
		else
		{
         linkEditor.setColorSource(NetworkMapLink.COLOR_SOURCE_DEFAULT);
		}
      linkEditor.setUseActiveThresholds(checkUseThresholds.getSelection());
      linkEditor.setUseInterfaceUtilization(checkUseUtilization.getSelection());
		linkEditor.setModified();
		return true;
	}
}
