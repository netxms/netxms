package org.netxms.agent.parameters;

import org.netxms.agent.Parameter;
import org.netxms.agent.ParameterType;

public class AgentVersion extends Parameter {

    public AgentVersion() {
        super("Agent.Version", "Agent Version", ParameterType.STRING);
    }

    @Override
    public String getValue(final String argument) {
        return "1.1.10";
    }
}
