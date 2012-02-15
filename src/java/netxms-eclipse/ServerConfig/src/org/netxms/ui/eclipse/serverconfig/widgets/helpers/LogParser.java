/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.widgets.helpers;

import java.util.List;
import java.util.Map;
import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.ElementMap;
import org.simpleframework.xml.Root;

/**
 * Log parser configuration
 */
@Root(name="parser", strict=false)
public class LogParser
{
	@Attribute(required=false)
	private int trace;
	
	@Element(required=false)
	private String file;
	
	@ElementList(required=false)
	private List<LogParserRule> rules;
	
	@ElementMap(entry="macro", key="name", attribute=true, required=false)
	private Map<String, String> macros;
}
