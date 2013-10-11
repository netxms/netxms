package org.netxms.agent;

public interface ListParameter {

	String getName();

	public String[] getValues(final String param);
}
