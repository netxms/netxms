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
package org.netxms.ui.eclipse.objectview.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.ui.eclipse.objectview.dialogs.PatchPanelSelectonDialog;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * Patch panel selector widget. Provides uniform way to display selected
 * patch panel and change selection.
 */
public class PatchPanelSelector extends AbstractSelector
{
   private Rack rack;
   private long patchPanelId = 0;
   private String patchPanelName = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public PatchPanelSelector(Composite parent, int style, Rack rack)
	{
		super(parent, style, 0);
		setText("None");
		this.rack = rack;
	}
	
	/**
	 * Change parent rack element
	 * @param rack parent rack for selected patch pannel
	 */
	public void setRack(Rack rack)
	{
	   if(this.rack != rack)
	      clearButtonHandler();
	   this.rack = rack;
	}

	/**
	 * @param parent
	 * @param style
	 */
	public PatchPanelSelector(Composite parent, int style, boolean useHyperlink)
	{
		super(parent, style, (useHyperlink ? USE_HYPERLINK : 0) | SHOW_CLEAR_BUTTON);
		setText("None");
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
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
				setText("None");
				getTextControl().setToolTipText(null);
			}
			if (prevPatchPanelId != patchPanelId)
				fireModifyListeners();
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#clearButtonHandler()
	 */
	@Override
	protected void clearButtonHandler()
	{
		if (patchPanelId == 0)
			return;
		
		patchPanelId = 0;
		patchPanelName = null;
		setText("None");
		setImage(null);
		getTextControl().setToolTipText(null);
		fireModifyListeners();
	}

	/**
	 * Get code of selected event
	 * 
	 * @return Selected event's code
	 */
	public long getPatchPanelId()
	{
		return patchPanelId;
	}

   /**
    * Get name of selected event
    * 
    * @return Selected event's name
    */
   public String getPatchPanelName()
   {
      return patchPanelName;
   }

	/**
	 * Set event code
	 * @param eventCode
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
				setText("Unknown");
				setImage(null);
				getTextControl().setToolTipText(null);
			}
		}
		else
		{
			setText("None");
			setImage(null);
			getTextControl().setToolTipText(null);
		}
		fireModifyListeners();
	}

	/**
	 * Generate tooltip text for event
	 * @param event event template
	 * @return tooltip text
	 */
	private String generateToolTipText(PassiveRackElement element)
	{
		StringBuilder sb = new StringBuilder(element.getName());
		sb.append(" ["); //$NON-NLS-1$
		sb.append(element.getId());
      sb.append("]\n\n"); //$NON-NLS-1$
		sb.append("Position: ");
		sb.append(element.getPosition());
      sb.append("\nOrientation:"); //$NON-NLS-1$
      sb.append(element.getOrientation().toString());
		return sb.toString();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#getSelectionButtonToolTip()
	 */
	@Override
	protected String getSelectionButtonToolTip()
	{
		return "Patch panel selector";
	}
}
