/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.MarginBorder;
import org.eclipse.draw2d.StackLayout;
import org.eclipse.draw2d.text.FlowPage;
import org.eclipse.draw2d.text.TextFlow;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.shared.StatusDisplayInfo;

/**
 * @author victor
 *
 */
public class ObjectTooltip extends Figure
{
	public ObjectTooltip(GenericObject object)
	{
		setBorder(new MarginBorder(3));
		setLayoutManager(new StackLayout());
		
		FlowPage page = new FlowPage();
		add(page);
		
		TextFlow text = new TextFlow();
		StringBuilder sb = new StringBuilder(object.getObjectName());
		sb.append("\nStatus: ");
		sb.append(StatusDisplayInfo.getStatusText(object.getStatus()));
		if ((object instanceof Node) && !((Node)object).getPrimaryIP().isAnyLocalAddress())
		{
			sb.append("Primary IP: ");
			sb.append(((Node)object).getPrimaryIP().toString());
			sb.append('\n');
		}
		if (!object.getComments().isEmpty())
		{
			sb.append("\n\n");
			sb.append(object.getComments());
		}
		text.setText(sb.toString());
		page.add(text);
	}
}
