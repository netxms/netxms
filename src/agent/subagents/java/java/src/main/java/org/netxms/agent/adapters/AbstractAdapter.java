package org.netxms.agent.adapters;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

abstract class AbstractAdapter {

    @SuppressWarnings("WeakerAccess")
    protected String extractFirstArgument(final String name) {
        List<String> arguments = extractArguments(name);
        if (arguments.size() > 0) {
            return arguments.get(0);
        }
        return "";
    }

    @SuppressWarnings("WeakerAccess")
    protected List<String> extractArguments(final String name) {
        int startIndex = name.indexOf("(");
        int endIndex = name.lastIndexOf(")");
        if (startIndex > 0 && endIndex > startIndex) {
            String value = name.substring(startIndex + 1, endIndex);
            ArrayList<String> arguments = new ArrayList<String>();
            for (String argument : value.split(",")) {
                arguments.add(argument.trim());
            }
            return arguments;
        }
        return Collections.emptyList();
    }
}
