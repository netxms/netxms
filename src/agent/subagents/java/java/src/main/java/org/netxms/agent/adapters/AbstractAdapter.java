package org.netxms.agent.adapters;

import org.netxms.agent.SubAgent;

abstract class AbstractAdapter {

    protected String getArgument(String name, int index) {
        return SubAgent.getParameterArg(name, index + 1);
    }

    protected Integer getArgumentAsInt(String name, int index) {
        String stringValue = SubAgent.getParameterArg(name, index + 1);
        try {
            return Integer.valueOf(stringValue);
        } catch (NumberFormatException ignored) {
        }
        return 0;
    }
}
