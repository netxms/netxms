package org.netxms.agent;

public abstract class ListParameter extends BaseParameter {

    /**
     * @param name        Parameter name WITHOUT brackets (for parameters with arguments)
     * @param description Parameter description
     * @param type        Return type
     */
    protected ListParameter(final String name, final String description, final ParameterType type) {
        super(name, description, type);
    }

    @Override
    public String getValue(final String argument) {
        return null;
    }
}
