package org.netxms.agent;

public abstract class xParameter extends Parameter {

    /**
     * @param name        Parameter name WITHOUT brackets (for parameters with arguments)
     * @param description Parameter description
     * @param type        Return type
     */
    protected xParameter(final String name, final String description, final ParameterType type) {
        super(name, description, type);
    }

    @Override
    public String[] getListValue(final String argument) {
        return null;
    }
}
