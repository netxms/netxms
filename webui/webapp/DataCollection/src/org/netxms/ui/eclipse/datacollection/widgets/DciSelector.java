/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.widgets;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.Node;
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
	private static final long serialVersionUID = 1L;

	private long nodeId = 0;
	private long dciId = 0;
	private String emptySelectionName = Messages.DciSelector_None;
	private NXCSession session;
	private int dcObjectType = -1;

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
		SelectDciDialog dlg = new SelectDciDialog(getShell());
		dlg.setDcObjectType(dcObjectType);
		if (dlg.open() == Window.OK)
		{
			DciValue dci = dlg.getSelection();
			setDciId(dci.getNodeId(), dci.getId());
		}
	}
	
	/**
	 * Update text
	 */
	private void updateText()
	{
		if (nodeId == 0)
		{
			setText(emptySelectionName);
			return;
		}
		
		new ConsoleJob(Messages.DciSelector_JobTitle, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final String[] names = session.resolveDciNames(new long[] { nodeId }, new long[] { DciSelector.this.dciId });
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						Node node = (Node)session.findObjectById(nodeId, Node.class);
						
						StringBuilder sb = new StringBuilder();
						if (node != null)
						{
							sb.append(node.getObjectName());
						}
						else
						{
							sb.append('[');
							sb.append(nodeId);
							sb.append(']');
						}
						sb.append(" / "); //$NON-NLS-1$
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
				return Messages.DciSelector_JobError;
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
}
