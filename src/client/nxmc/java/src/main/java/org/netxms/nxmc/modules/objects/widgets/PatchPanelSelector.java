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
package org.netxms.nxmc.modules.objects.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.PatchPanelSelectonDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Patch panel selector widget. Provides uniform way to display selected
 * patch panel and change selection.
 */
public class PatchPanelSelector extends AbstractSelector
{
   private final I18n i18n = LocalizationHelper.getI18n(PatchPanelSelector.class);

   private Rack rack;
   private long patchPanelId = 0;
   private String patchPanelName = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public PatchPanelSelector(Composite parent, int style, Rack rack)
	{
      this(parent, style, rack, false);
	}

   /**
    * @param parent
    * @param style
    */
   public PatchPanelSelector(Composite parent, int style, boolean useHyperlink)
   {
      this(parent, style, null, useHyperlink);
   }

	/**
	 * @param parent
	 * @param style
	 */
   public PatchPanelSelector(Composite parent, int style, Rack rack, boolean useHyperlink)
	{
		super(parent, style, new SelectorConfigurator()
		      .setUseHyperlink(useHyperlink)
		      .setShowClearButton(true)
		      .setSelectionButtonToolTip(LocalizationHelper.getI18n(PatchPanelSelector.class).tr("Select patch panel")));

      this.rack = rack;
      setText(i18n.tr("None"));
	}

   /**
    * Change parent rack
    *
    * @param rack new parent rack
    */
   public void setRack(Rack rack)
   {
      if (this.rack != rack)
         clearButtonHandler();
      this.rack = rack;
   }

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractSelector#selectionButtonHandler()
    */
	@Override
	protected void selectionButtonHandler()
	{
	   PatchPanelSelectonDialog dlg = new PatchPanelSelectonDialog(getShell(), rack);
		if (dlg.open() == Window.OK)
		{
			long prevPatchPanelId = patchPanelId;
			long id = dlg.getPatchPanelId();
			if (id != 0)
			{
			   patchPanelId = id;
			   PassiveRackElement element = rack.getPassiveElement(id);
				patchPanelName = rack.getPassiveElement(id).toString();
				setText(patchPanelName);
            getTextControl().setToolTipText(generateToolTipText(element));
			}
			else
			{
			   patchPanelId = 0;
				patchPanelName = null;
            setText(i18n.tr("None"));
				getTextControl().setToolTipText(null);
			}
			if (prevPatchPanelId != patchPanelId)
				fireModifyListeners();
		}
	}

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractSelector#clearButtonHandler()
    */
	@Override
	protected void clearButtonHandler()
	{
		if (patchPanelId == 0)
			return;

		patchPanelId = 0;
		patchPanelName = null;
      setText(i18n.tr("None"));
		setImage(null);
		getTextControl().setToolTipText(null);
		fireModifyListeners();
	}

	/**
    * Get ID of selected patch panel
    * 
    * @return ID of selected patch panel
    */
	public long getPatchPanelId()
	{
		return patchPanelId;
	}

   /**
    * Get name of selected patch panel
    * 
    * @return name of selected patch panel
    */
   public String getPatchPanelName()
   {
      return patchPanelName;
   }

	/**
    * Set patch panel ID
    *
    * @param ID of selected patch panel
    */
	public void setPatchPanelId(long patchPanelId)
	{
		if (this.patchPanelId == patchPanelId)
			return;	// nothing to change
		
		this.patchPanelId = patchPanelId;
		if (patchPanelId != 0)
		{
         PassiveRackElement element = rack.getPassiveElement(patchPanelId);
			if (element != null)
			{
			   patchPanelName = element.toString();
				setText(patchPanelName);
				getTextControl().setToolTipText(generateToolTipText(element));
			}
			else
			{
            setText(i18n.tr("<unknown>"));
				setImage(null);
				getTextControl().setToolTipText(null);
			}
		}
		else
		{
         setText(i18n.tr("None"));
			setImage(null);
			getTextControl().setToolTipText(null);
		}
		fireModifyListeners();
	}

	/**
    * Generate tooltip text for element
    *
    * @param element passive rack element
    * @return tooltip text
    */
	private String generateToolTipText(PassiveRackElement element)
	{
		StringBuilder sb = new StringBuilder(element.getName());
      sb.append(" [");
		sb.append(element.getId());
      sb.append("]\n\n");
		sb.append("Position: ");
		sb.append(element.getPosition());
      sb.append("\nOrientation:");
      sb.append(element.getOrientation().toString());
		return sb.toString();
	}
}
