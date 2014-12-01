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
package org.netxms.ui.eclipse.datacollection.widgets;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.dialogs.SelectDciDialog;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * DCI selection widget
 */
public class DciSelector extends AbstractSelector
{
	private long nodeId = 0;
	private long dciId = 0;
	private String emptySelectionName = Messages.get().DciSelector_None;
	private NXCSession session;
	private int dcObjectType = -1;
   private int dciObjectType = -1;
	private String dciName = null;
	private boolean fixedNode = false;
	private boolean allowNoValueObjects = false;

	/**
	 * @param parent
	 * @param style
	 * @param useHyperlink
	 */
	public DciSelector(Composite parent, int style, boolean useHyperlink)
	{
		super(parent, style, USE_TEXT | (useHyperlink ? USE_HYPERLINK : 0));
		setText(emptySelectionName);
		session = (NXCSession)ConsoleSharedData.getSession();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		SelectDciDialog dlg = new SelectDciDialog(getShell(), fixedNode ? nodeId : 0);
		dlg.setEnableEmptySelection(true);
		dlg.setDcObjectType(dcObjectType);
		dlg.setSingleSelection(true);
		dlg.setAllowNoValueObjects(allowNoValueObjects);
		if (dlg.open() == Window.OK)
		{
		   List<DciValue> dci = dlg.getSelection();
			if (dci != null && dci.size() == 1)
			{
				setDciId(dci.get(0).getNodeId(), dci.get(0).getId());
				dciName = dci.get(0).getName();
				dciObjectType = dci.get(0).getDcObjectType();
			}
			else
			{
				setDciId(fixedNode ? nodeId : 0, 0);
				dciName = null;
			}
		}
	}
	
	/**
	 * Update text
	 */
	private void updateText()
	{
		if ((nodeId == 0) || (dciId == 0))
		{
			setText(emptySelectionName);
			return;
		}
		
		new ConsoleJob(Messages.get().DciSelector_JobTitle, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final String[] names = session.resolveDciNames(new long[] { nodeId }, new long[] { DciSelector.this.dciId });
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						AbstractObject object = session.findObjectById(nodeId);
						
						StringBuilder sb = new StringBuilder();
						if (!fixedNode)
						{
							if (object != null)
							{
								sb.append(object.getObjectName());
							}
							else
							{
								sb.append('[');
								sb.append(nodeId);
								sb.append(']');
							}
							sb.append(" / "); //$NON-NLS-1$
						}
						if (names.length > 0)
						{
							sb.append(names[0]);
						}
						else
						{
							sb.append('[');
							sb.append(dciId);
							sb.append(']');
						}
						
						setText(sb.toString());
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().DciSelector_JobError;
			}
		}.start();
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @return the dciId
	 */
	public long getDciId()
	{
		return dciId;
	}

	/**
	 * @param dciId
	 */
	public void setDciId(long nodeId, long dciId)
	{
		this.nodeId = nodeId;
		this.dciId = dciId;
		updateText();
	}

	/**
	 * @return the emptySelectionName
	 */
	public String getEmptySelectionName()
	{
		return emptySelectionName;
	}

	/**
	 * @param emptySelectionName the emptySelectionName to set
	 */
	public void setEmptySelectionName(String emptySelectionName)
	{
		this.emptySelectionName = emptySelectionName;
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
	}

	/**
	 * @return the fixedNode
	 */
	public final boolean isFixedNode()
	{
		return fixedNode;
	}

	/**
	 * @param fixedNode the fixedNode to set
	 */
	public final void setFixedNode(boolean fixedNode)
	{
		this.fixedNode = fixedNode;
	}

	/**
	 * @return the dciName
	 */
	public final String getDciName()
	{
		return dciName;
	}
	
	  /**
    * @return the dciName
    */
   public final String getDciToolTipInfo()
   {
       return getText();
   }

   /**
    * @return the dciObjectType
    */
   public int getDciObjectType()
   {
      return dciObjectType;
   }

   /**
    * @param dciObjectType the dciObjectType to set
    */
   public void setDciObjectType(int dciObjectType)
   {
      this.dciObjectType = dciObjectType;
   }

   /**
    * @param allowNoValueObjects the allowNoValueObjects to set
    */
   public void setAllowNoValueObjects(boolean allowNoValueObjects)
   {
      this.allowNoValueObjects = allowNoValueObjects;
   }

   /**
    * @return the allowNoValueObjects
    */
   public boolean isAllowNoValueObjects()
   {
      return allowNoValueObjects;
   }
}
