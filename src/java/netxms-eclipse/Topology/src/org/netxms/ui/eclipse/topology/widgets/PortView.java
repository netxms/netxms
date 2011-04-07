/**
 * 
 */
package org.netxms.ui.eclipse.topology.widgets;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortInfo;

/**
 * View of switch/router ports
 *
 */
public class PortView extends Canvas implements PaintListener
{
	private long nodeId;
	private NXCSession session;
	private Map<Long, PortInfo> ports = new HashMap<Long, PortInfo>();
	
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
		
		ports.clear();
		Set<GenericObject> interfaces = object.getAllChilds(GenericObject.OBJECT_INTERFACE);
		for(GenericObject o : interfaces)
		{
			ports.put(o.getObjectId(), new PortInfo((Interface)o));
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
