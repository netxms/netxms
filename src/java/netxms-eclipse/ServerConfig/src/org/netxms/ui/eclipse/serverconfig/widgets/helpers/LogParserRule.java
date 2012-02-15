/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.widgets.helpers;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * Log parser rule
 */
@Root(name="rule", strict=false)
public class LogParserRule
{
	@Attribute(required=false)
	private String context;
	
	@Attribute(name="break", required=false)
	private boolean breakProcessing;
	
	@Element(required=false)
	private String match;
	
	@Element(required=false)
	private LogParserEvent event;
	
	@Element(required=false)
	private int severity;
	
	@Element(required=false)
	private int facility;
	
	@Element(required=false)
	private String tag;
}
