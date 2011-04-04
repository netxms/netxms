/**
 * 
 */
package org.netxms.ui.eclipse.topology.widgets;

import java.util.Set;

import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;

/**
 * View of switch/router ports
 *
 */
public class PortView extends Composite implements PaintListener
{
	private long nodeId;
	private NXCSession session;
	
	/**
	 * @param parent
	 * @param style
	 */
	public PortView(Composite parent, int style)
	{
		super(parent, style);
		addPaintListener(this);
		refresh();
	}
	
	/**
	 * Refresh widget
	 */
	public void refresh()
	{
		GenericObject object = session.findObjectById(nodeId);
		if ((object == null) || !(object instanceof Node))
			return;
		
		Set<GenericObject> interfaces = object.getAllChilds(GenericObject.OBJECT_INTERFACE);
		for(GenericObject o : interfaces)
		{
			
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		
	}
}
