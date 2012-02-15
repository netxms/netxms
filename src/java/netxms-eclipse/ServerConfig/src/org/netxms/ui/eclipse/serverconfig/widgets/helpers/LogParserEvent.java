/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.widgets.helpers;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Text;

/**
 * Event in log parser rule
 */
@Root(name="event", strict=false)
public class LogParserEvent
{
	@Attribute(name="params", required=false)
	private int parameterCount;
	
	@Text
	private String event;
}
