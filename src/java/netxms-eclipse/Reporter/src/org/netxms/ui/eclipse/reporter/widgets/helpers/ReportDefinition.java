/**
 * 
 */
package org.netxms.ui.eclipse.reporter.widgets.helpers;

import java.util.List;

import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Jasper report definition 
 */
@Root(name="jasperReport", strict=false)
public class ReportDefinition
{
	@ElementList(inline=true)
	private List<ReportParameter> parameters;

	/**
	 * Create report definition object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static ReportDefinition createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(ReportDefinition.class, xml.replaceAll("class=\"([a-zA-Z0-9\\._]+)\"", "javaClass=\"$1\""));
	}

	/**
	 * @return the parameters
	 */
	public List<ReportParameter> getParameters()
	{
		return parameters;
	}
}
