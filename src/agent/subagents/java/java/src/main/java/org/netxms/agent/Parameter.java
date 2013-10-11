package org.netxms.agent;

import org.netxms.agent.ParameterType;

public interface Parameter {

	String getName();
	String getDescription();
	ParameterType getType();
	
	String getValue(final String param);
}
