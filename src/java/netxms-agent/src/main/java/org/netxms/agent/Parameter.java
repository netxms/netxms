package org.netxms.agent;

public abstract class Parameter extends BaseParameter {

    /**
     * @param name        Parameter name WITHOUT brackets (for parameters with arguments)
     * @param description Parameter description
     * @param type        Return type
     */
    protected Parameter(final String name, final String description, final ParameterType type) {
        super(name, description, type);
    }

    @Override
    public String[] getListValue(final String argument) {
        return null;
    }
}
