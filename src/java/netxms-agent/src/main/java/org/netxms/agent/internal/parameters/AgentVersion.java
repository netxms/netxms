package org.netxms.agent.internal.parameters;

import org.netxms.agent.ItemParameter;
import org.netxms.agent.ParameterType;

public class AgentVersion extends ItemParameter {

    public AgentVersion() {
        super("Agent.Version", "Agent Version", ParameterType.STRING);
    }

    @Override
    public String getValue(final String argument) {
        return "1.1.10";
    }
}
