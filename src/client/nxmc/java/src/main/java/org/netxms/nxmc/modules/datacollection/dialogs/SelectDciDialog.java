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
package org.netxms.nxmc.modules.datacollection.dialogs;

import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.Template;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.DciList;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ObjectTree;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for DCI selection
 */
public class SelectDciDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(SelectDciDialog.class);

	private SashForm splitter;
	private ObjectTree objectTree;
	private DciList dciList;
	private List<DciValue> selection;
	private int dcObjectType = -1;
	private long fixedNode;
	private boolean enableEmptySelection = false;
	private boolean allowTemplateItems = false;
	private boolean allowSingleSelection = false;
	private boolean allowNoValueObjects = false;
	
	/**
	 * @param parentShell
	 */
	public SelectDciDialog(Shell parentShell, long fixedNode)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		this.fixedNode = fixedNode;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Select DCI"));
      PreferenceStore settings = PreferenceStore.getInstance();
      newShell.setSize(settings.getAsInteger("SelectDciDialog.width", 600), settings.getAsInteger("SelectDciDialog.height", 350));
      newShell.setLocation(settings.getAsInteger("SelectDciDialog.cx", 100), settings.getAsInteger("SelectDciDialog.cy", 100));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected void createButtonsForButtonBar(Composite parent)
	{
		if (enableEmptySelection)
		{
         Button button = createButton(parent, 1000, i18n.tr("&None"), false);
         button.addSelectionListener(new SelectionAdapter() {
				@Override
				public void widgetSelected(SelectionEvent e)
				{
					selection = null;
					saveSettings();
					SelectDciDialog.super.okPressed();
				}
			});
		}		
		super.createButtonsForButtonBar(parent);
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
      PreferenceStore settings = PreferenceStore.getInstance();

		Composite dialogArea = (Composite)super.createDialogArea(parent);
		dialogArea.setLayout(new FillLayout());

		if (fixedNode == 0)
		{
			splitter = new SashForm(dialogArea, SWT.HORIZONTAL);

         objectTree = new ObjectTree(splitter, SWT.BORDER, false,
               allowTemplateItems ? ObjectSelectionDialog.createDataCollectionOwnerSelectionFilter() : ObjectSelectionDialog.createDataCollectionTargetSelectionFilter(), 
                     null, null, true, false);
         String text = settings.getAsString("SelectDciDialog.Filter");
			if (text != null)
				objectTree.setFilterText(text);
		}

      dciList = new DciList(null, (fixedNode == 0) ? splitter : dialogArea, SWT.BORDER, null,
            "SelectDciDialog.dciList", dcObjectType, allowSingleSelection ? SWT.NONE : SWT.MULTI, allowNoValueObjects); //$NON-NLS-1$
		dciList.setDcObjectType(dcObjectType);
		dciList.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				okPressed();
			}
		});

		if (fixedNode == 0)
		{
         int[] weights = new int[2];
         weights[0] = settings.getAsInteger("SelectDciDialog.weight1", 30);
         weights[1] = settings.getAsInteger("SelectDciDialog.weight2", 70);
         splitter.setWeights(weights);

			objectTree.getTreeViewer().addSelectionChangedListener(new ISelectionChangedListener()	{
				@Override
				public void selectionChanged(SelectionChangedEvent event)
				{
					AbstractObject object = objectTree.getFirstSelectedObject2();
               if ((object != null) && ((object instanceof DataCollectionTarget) || (allowTemplateItems && (object instanceof Template))))
					{
						dciList.setNode(object);
					}
					else
					{
						dciList.setNode(null);
					}
				}
			});

         int objectId = settings.getAsInteger("SelectDciDialog.selectionNode", 0);
         if (objectId != 0)
         {
            AbstractObject object = Registry.getSession().findObjectById(objectId);
            if (object != null)
               objectTree.selectObject(object);
         }
			objectTree.setFocus();
		}
		else
		{
         dciList.setNode(Registry.getSession().findObjectById(fixedNode));
		}

		return dialogArea;
	}

	/**
	 * Save dialog settings
	 */
	private void saveSettings()
	{
		Point size = getShell().getSize();
		Point pleace = getShell().getLocation();
      PreferenceStore settings = PreferenceStore.getInstance();

      settings.set("SelectDciDialog.cx", pleace.x);
      settings.set("SelectDciDialog.cy", pleace.y);
      settings.set("SelectDciDialog.width", size.x);
      settings.set("SelectDciDialog.height", size.y);
		if (fixedNode == 0)
		{
         settings.set("SelectDciDialog.Filter", objectTree.getFilterText());

			int[] weights = splitter.getWeights();
         settings.set("SelectDciDialog.weight1", weights[0]);
         settings.set("SelectDciDialog.weight2", weights[1]);

         settings.set("SelectDciDialog.selectionNode", objectTree.getFirstSelectedObject()); 
		}
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		selection = dciList.getSelection();
		if (selection == null || selection.size() == 0)
		{
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please select DCI from the list and then press OK"));
			return;
		}
		saveSettings();
		super.okPressed();
	}

	/**
	 * @return the selection
	 */
	public List<DciValue> getSelection()
	{
		return selection;
	}

	/**
	 * @return the dcObjectType
	 */
	public int getDcObjectType()
	{
		return dcObjectType;
	}

	/**
	 * @param dcObjectType the dcObjectType to set
	 */
	public void setDcObjectType(int dcObjectType)
	{
		this.dcObjectType = dcObjectType;
		if (dciList != null)
			dciList.setDcObjectType(dcObjectType);
	}

	/**
	 * @return the enableEmptySelection
	 */
	public final boolean isEnableEmptySelection()
	{
		return enableEmptySelection;
	}

	/**
	 * @param enableEmptySelection the enableEmptySelection to set
	 */
	public final void setEnableEmptySelection(boolean enableEmptySelection)
	{
		this.enableEmptySelection = enableEmptySelection;
	}

	/**
	 * @return the allowTemplateItems
	 */
	public final boolean isAllowTemplateItems()
	{
		return allowTemplateItems;
	}

	/**
	 * @param allowTemplateItems the allowTemplateItems to set
	 */
	public final void setAllowTemplateItems(boolean allowTemplateItems)
	{
		this.allowTemplateItems = allowTemplateItems;
	}

	/**
    * @param allowSingleSelection false to have multiple selection, true to have single selection
    * in DCI list. 
    * By default multiple selection is set. 
    */
   public void setSingleSelection(boolean allowSingleSelection)
   {
      this.allowSingleSelection = allowSingleSelection;
   }

   /**
    * @param allowNoValueObjects the allowNoValueObjects to set
    */
   public void setAllowNoValueObjects(boolean allowNoValueObjects)
   {
      this.allowNoValueObjects = allowNoValueObjects;
   }
}
