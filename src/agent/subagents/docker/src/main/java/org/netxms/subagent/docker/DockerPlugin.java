package org.netxms.subagent.docker;

import com.amihaiemil.docker.Container;
import com.amihaiemil.docker.Docker;
import com.amihaiemil.docker.LocalDocker;
import org.netxms.agent.*;
import org.netxms.agent.adapters.ListParameterAdapter;
import org.netxms.agent.adapters.ParameterAdapter;
import org.netxms.bridge.Config;
import org.netxms.bridge.LogLevel;
import org.netxms.bridge.Platform;
import org.netxms.subagent.docker.adapters.ContainerListTableAdapter;

import java.io.File;
import java.io.IOException;
import java.util.*;

public class DockerPlugin extends Plugin {

    private final Docker docker;

    /**
     * Constructor used by PluginManager
     *
     * @param config agent configuration
     */
    public DockerPlugin(Config config) {
        super(config);

        docker = new LocalDocker(
                new File("/var/run/docker.sock")
        );

        try {
            boolean available = docker.ping();
            if (available) {
                Platform.writeLog(LogLevel.INFO, "Docker connected");
            } else {
                Platform.writeLog(LogLevel.WARNING, "Docker is not available");
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public String getName() {
        return "Docker";
    }

    public String getVersion() {
        return "1.0-SNAPSHOT";
    }

    @Override
    public Parameter[] getParameters() {
        Set<Parameter> parameters = new HashSet<>();

        parameters.add(new ParameterAdapter("Docker.Container.State(*)", "Docker container {instance} state", ParameterType.STRING) {
            @Override
            public String getValue(String param) throws Exception {
                String argument = SubAgent.getParameterArg(param, 1);
                Iterator<Container> containerIterator = docker.containers().all();
                while (containerIterator.hasNext()) {
                    Container container = containerIterator.next();
                    String name = container.inspect().getString("Name");
                    if (container.containerId().equalsIgnoreCase(argument) || name.equalsIgnoreCase(argument)) {
                        return container.getString("State");
                    }
                }
                return null;
            }
        });

        return parameters.toArray(new Parameter[]{});
    }

    @Override
    public ListParameter[] getListParameters() {
        Set<ListParameter> parameters = new HashSet<ListParameter>();

        parameters.add(new ListParameterAdapter("Docker.Containers", "List of all containers") {
            @Override
            public String[] getValue(String param) throws Exception {
                Iterator<Container> allContainers = docker.containers().all();
                List<String> names = new ArrayList<>();
                while (allContainers.hasNext()) {
                    Container container = allContainers.next();
                    names.add(container.inspect().getString("Name"));
                }
                return names.toArray(new String[]{});
            }
        });

        parameters.add(new ListParameterAdapter("Docker.RunningContainers", "List of running containers") {
            @Override
            public String[] getValue(String param) throws Exception {
                List<String> names = new ArrayList<>();
                for (Container container : docker.containers()) {
                    names.add(container.inspect().getString("Name"));

                }
                return names.toArray(new String[]{});
            }
        });

        return parameters.toArray(new ListParameter[]{});
    }

    @Override
    public TableParameter[] getTableParameters() {
        Set<TableParameter> parameters = new HashSet<>();
        parameters.add(new ContainerListTableAdapter(docker));

        return parameters.toArray(new TableParameter[]{});
    }
}
