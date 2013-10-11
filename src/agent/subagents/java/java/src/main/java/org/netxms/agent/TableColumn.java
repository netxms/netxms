package org.netxms.agent;

public class TableColumn {
	private String name;
	private ParameterType type;
	private boolean instance;
		
	public TableColumn(final String name, final ParameterType type, final boolean instance) {
		this.name = name;
		this.type = type;
		this.instance = instance;
	}

	public String getName() {
		return this.name;
	}

	public ParameterType getType() {
		return this.type;
	}

	public boolean isInstance() {
		return this.instance;
	}
}
