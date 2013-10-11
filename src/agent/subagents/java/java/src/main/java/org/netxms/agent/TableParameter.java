package org.netxms.agent;

import org.netxms.agent.ParameterType;

public interface TableParameter {

	String getName();
	String getDescription();

	TableColumn[] getColumns();
	String[][] getValues(final String param);
}
