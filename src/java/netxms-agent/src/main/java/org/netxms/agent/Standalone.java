package org.netxms.agent;

import org.netxms.agent.parameters.AgentVersion;
import org.netxms.agent.transport.SocketConnector;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class Standalone implements ParameterProvider {

    private final Logger log = LoggerFactory.getLogger(Standalone.class);
    private static final int PORT = 4700;

    public static void main(final String[] args) {
        new Standalone().start(args);
    }

    private void start(final String[] args) {
        final SocketConnector connector = new SocketConnector(PORT);
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
    public List<BaseParameter> getParameters() {
        final List<BaseParameter> parameters = new ArrayList<BaseParameter>(3);
        parameters.add(new AgentVersion());
        return parameters;
    }

}
