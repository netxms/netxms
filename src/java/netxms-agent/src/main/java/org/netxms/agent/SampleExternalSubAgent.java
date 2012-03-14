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

import org.netxms.agent.transport.Connector;
import org.netxms.agent.transport.PipeConnector;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class SampleExternalSubAgent implements ItemParameterProvider, ListProvider {
    public static void main(final String[] args) throws IOException {
        new SampleExternalSubAgent().start();
    }

    private void start() throws IOException {
        final Connector connector = new PipeConnector("java");
        final Engine engine = new Engine(connector);
        engine.registerParameterProvider(this);
        engine.registerListParameterProvider(this);
        engine.start();

        System.out.println("Started, press <Enter> to stop");

        //noinspection ResultOfMethodCallIgnored
        System.in.read();

        System.out.println("Stopping");

        engine.stop();
    }

    @Override
    public List<Parameter> getListParameters() {
        return new ArrayList<Parameter>(0);
    }

    @Override
    public List<Parameter> getItemParameters() {
        final List<Parameter> parameters = new ArrayList<Parameter>(3);
        parameters.add(new ItemParameter("Java.Parameter1", "Description 1", ParameterType.INT) {
            @Override
            public String getValue(final String argument) {
                return "123";
            }
        });
        parameters.add(new ItemParameter("Java.Parameter2", "Description 2", ParameterType.STRING) {
            @Override
            public String getValue(final String argument) {
                return "some string";
            }
        });
        parameters.add(new ListParameter("Java.List", "Sample List", ParameterType.STRING) {
            @Override
            public String[] getListValue(final String argument) {
                return new String[]{"line1", "line2", "line3"};
            }
        });
        return parameters;
    }
}
