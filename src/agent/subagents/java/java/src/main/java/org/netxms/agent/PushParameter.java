package org.netxms.agent;

import org.netxms.agent.ParameterType;

public interface PushParameter {

	String getName();
	String getDescription();
	ParameterType getType();

}
