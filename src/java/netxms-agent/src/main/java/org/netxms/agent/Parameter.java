/**
 * NetXMS - open source network management system
 * Copyright (C) 2012 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.agent;

/**
 * Agent's parameter
 */
public abstract class Parameter {

    private String name;
    private String description;
    private ParameterType type;

    /**
     * @param name        Parameter name WITHOUT brackets (for parameters with arguments)
     * @param description Parameter description
     * @param type        Return type
     */
    protected Parameter(final String name, final String description, final ParameterType type) {
        this.name = name;
        this.description = description;
        this.type = type;
    }

    /**
     * @return the name
     */
    public String getName() {
        return name;
    }

    /**
     * @return the description
     */
    public String getDescription() {
        return description;
    }

    /**
     * @return the type
     */
    public ParameterType getType() {
        return type;
    }

    /**
     * Value provider for parameter
     *
     * @param argument Argument provided by user. Can be empty string for parameter without arguments
     * @return String value or null in case of error
     */
    public abstract String getValue(final String argument);

    /**
     * Value provider for list argument
     *
     * @param argument Argument provided by user. Can be empty string for parameter without arguments
     * @return List of String values or null in case of error
     */
    public abstract String[] getListValue(final String argument);

}
