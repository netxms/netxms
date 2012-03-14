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

import org.netxms.agent.internal.MessageConsumer;
import org.netxms.agent.internal.RCC;
import org.netxms.agent.transport.Connector;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public final class Engine implements MessageConsumer {

    private final Logger log = LoggerFactory.getLogger(Engine.class);

    private class ArgumentParser {
        private String request;
        private String parameterName;
        private String argument;

        private ArgumentParser(final String request) {
            this.request = request;
        }

        public String getParameterName() {
            return parameterName;
        }

        public String getArgument() {
            return argument;
        }

        public ArgumentParser invoke() {
            final int leftBracket = request.indexOf('(');
            if (leftBracket != -1) {
                parameterName = request.substring(0, leftBracket).toLowerCase();
                final int rightBracket = request.indexOf(')');
                if (rightBracket != -1) {
                    argument = request.substring(leftBracket, rightBracket);
                } else {
                    argument = request.substring(leftBracket);
                }
            } else {
                parameterName = request.toLowerCase();
                argument = "";
            }
            return this;
        }
    }

    private final Connector connector;
    private final Set<ItemParameterProvider> parameterProviders = new HashSet<ItemParameterProvider>(0);
    private final Set<ListProvider> listProviders = new HashSet<ListProvider>(0);
    private Map<String, Parameter> parameterMap = new HashMap<String, Parameter>(0);
    private Map<String, Parameter> listMap = new HashMap<String, Parameter>(0);

    public Engine(final Connector connector) {
        this.connector = connector;
    }

    void start() {
        connector.setMessageConsumer(this);
        connector.start();
    }

    void stop() {
        connector.stop();
    }

    void registerParameterProvider(final ItemParameterProvider provider) {
        for (final Parameter parameter : provider.getItemParameters()) {
            parameterMap.put(parameter.getName().toLowerCase(), parameter);
        }
        parameterProviders.add(provider);
    }

    void registerListParameterProvider(final ListProvider provider) {
        for (final Parameter parameter : provider.getListParameters()) {
            listMap.put(parameter.getName().toLowerCase(), parameter);
        }
        listProviders.add(provider);
    }

    @Override
    public void processMessage(final NXCPMessage message) {
        final int messageCode = message.getMessageCode();
        log.debug("Message received: {}", messageCode);
        final NXCPMessage response = new NXCPMessage(NXCPCodes.CMD_REQUEST_COMPLETED, message.getMessageId());
        //noinspection CatchGenericClass
        try {
            internalProcessMessage(message, response);
        } catch (Exception e) {
            log.error("Command processing produced exception", e);
            response.setVariableInt32(NXCPCodes.VID_RCC, RCC.INTERNAL_ERROR);
        }
        connector.sendMessage(response);
    }

    private void internalProcessMessage(final NXCPMessage message, final NXCPMessage response) {
        switch (message.getMessageCode()) {
            case NXCPCodes.CMD_KEEPALIVE:
                response.setVariableInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
                break;
            case NXCPCodes.CMD_GET_PARAMETER_LIST:
                processGetParameterList(response);
                break;
            case NXCPCodes.CMD_GET_PARAMETER: {
                final String request = message.getVariableAsString(NXCPCodes.VID_PARAMETER);
                processGetParameter(response, request);
                break;
            }
            case NXCPCodes.CMD_GET_LIST: {
                final String request = message.getVariableAsString(NXCPCodes.VID_PARAMETER);
                processGetList(response, request);
                break;
            }
            case NXCPCodes.CMD_GET_TABLE:
                // unsupported
            default:
                response.setVariableInt32(NXCPCodes.VID_RCC, RCC.UNKNOWN_COMMAND);
                break;
        }
    }

    private void processGetParameterList(final NXCPMessage response) {
        response.setVariableInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
        fillParameterListMessage(response);
        connector.sendMessage(response);
    }

    private void fillParameterListMessage(final NXCPMessage response) {
        final List<Parameter> parameters = new ArrayList<Parameter>(0);
        for (final ItemParameterProvider parameterProvider : parameterProviders) {
            parameters.addAll(parameterProvider.getItemParameters());
        }

        response.setVariableInt32(NXCPCodes.VID_NUM_PARAMETERS, parameters.size());
        long variableId = NXCPCodes.VID_PARAM_LIST_BASE;
        for (final Parameter parameter : parameters) {
            response.setVariable(variableId++, parameter.getName());
            response.setVariable(variableId++, parameter.getDescription());
            response.setVariableInt16(variableId++, parameter.getType().getValue());
        }
    }

    private void processGetParameter(final NXCPMessage response, final String request) {
        final ArgumentParser argumentParser = new ArgumentParser(request).invoke();
        final String parameterName = argumentParser.getParameterName();
        final String argument = argumentParser.getArgument();

        if (parameterMap.containsKey(parameterName)) {
            final Parameter parameter = parameterMap.get(parameterName);
            final String value = parameter.getValue(argument);
            if (value != null) {
                response.setVariableInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
                response.setVariable(NXCPCodes.VID_VALUE, value);
            } else {
                response.setVariableInt32(NXCPCodes.VID_RCC, RCC.INTERNAL_ERROR);
            }
        } else {
            response.setVariableInt32(NXCPCodes.VID_RCC, RCC.UNKNOWN_PARAMETER);
        }
    }

    private void processGetList(final NXCPMessage response, final String request) {
        final ArgumentParser argumentParser = new ArgumentParser(request).invoke();
        final String parameterName = argumentParser.getParameterName();
        final String argument = argumentParser.getArgument();

        if (listMap.containsKey(parameterName)) {
            final Parameter parameter = listMap.get(parameterName);
            final String[] value = parameter.getListValue(argument);
            if (value != null) {
                response.setVariableInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
                response.setVariableInt32(NXCPCodes.VID_NUM_STRINGS, value.length);
                for (int i = 0, valueLength = value.length; i < valueLength; i++) {
                    response.setVariable(NXCPCodes.VID_ENUM_VALUE_BASE + (long) i, value[i]);
                }
            } else {
                response.setVariableInt32(NXCPCodes.VID_RCC, RCC.INTERNAL_ERROR);
            }
        } else {
            response.setVariableInt32(NXCPCodes.VID_RCC, RCC.UNKNOWN_PARAMETER);
        }
    }
}
