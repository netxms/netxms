package org.netxms.agent;

public interface Action {

	String getName();
	String getDescription();
	
	void execute(final String action, final String[] args);  
}
