package org.netxms.ui.eclipse.dashboard.widgets.internal;

import java.io.StringWriter;
import java.io.Writer;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementArray;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

@Root(name = "element")
public class StatusIndicatorConfig extends DashboardElementConfig
{

	@ElementArray(required = true)
	private ConditionInfo[] conditionList = new ConditionInfo[0];

	@Element(required = false)
	private String title = "";

	/**
	 * Create line chart settings object from XML document
	 * 
	 * @param xml
	 *           XML document
	 * @return deserialized object
	 * @throws Exception
	 *            if the object cannot be fully deserialized
	 */
	public static StatusIndicatorConfig createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(StatusIndicatorConfig.class, xml);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#createXml()
	 */
	@Override
	public String createXml() throws Exception
	{
		Serializer serializer = new Persister();
		Writer writer = new StringWriter();
		serializer.write(this, writer);
		return writer.toString();
	}

	/**
	 * @return the conditions
	 */
	public ConditionInfo[] getConditionList()
	{
		return conditionList;
	}

	/**
	 * @param conditions
	 *           the conditions to set
	 */
	public void setConditionList(ConditionInfo[] conditionList)
	{
		this.conditionList = conditionList;
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @param title
	 *           the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
	}
}
