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

import org.netxms.agent.internal.parameters.AgentVersion;
import org.netxms.agent.transport.Connector;
import org.netxms.agent.transport.SocketConnector;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public final class Standalone implements ItemParameterProvider {

    private final Logger log = LoggerFactory.getLogger(Standalone.class);
    private static final int PORT = 4700;

    public static void main(final String[] args) {
        new Standalone().start(args);
    }

    private void start(final String[] args) {
        final Connector connector = new SocketConnector(PORT);
        final Engine engine = new Engine(connector);

        engine.registerParameterProvider(this);

        engine.start();

        System.out.println("Started, press <Enter> to stop");

        try {
            //noinspection ResultOfMethodCallIgnored
            System.in.read();
        } catch (IOException e) {
            // ignore
        }

        System.out.println("Stopping");

        engine.stop();
    }

    @Override
    public List<Parameter> getItemParameters() {
        final List<Parameter> parameters = new ArrayList<Parameter>(3);
        parameters.add(new AgentVersion());
        return parameters;
    }

}
