/* 
 ** Java-Bridge NetXMS subagent
 ** Copyright (C) 2013 TEMPEST a.s.
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **
 ** File: TableColumn.java
 **
 **/

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
