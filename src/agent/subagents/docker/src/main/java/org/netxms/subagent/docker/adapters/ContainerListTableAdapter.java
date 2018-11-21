package org.netxms.subagent.docker.adapters;

import com.amihaiemil.docker.Container;
import com.amihaiemil.docker.Docker;
import org.netxms.agent.ParameterType;
import org.netxms.agent.TableColumn;
import org.netxms.agent.TableParameter;
import org.netxms.agent.adapters.TableAdapter;

import javax.json.JsonObject;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

public class ContainerListTableAdapter implements TableParameter {

    private final Docker docker;

    public ContainerListTableAdapter(Docker docker) {
        this.docker = docker;
    }

    @Override
    public TableColumn[] getColumns() {
        List<TableColumn> columns = new ArrayList<>();
        columns.add(new TableColumn("container_id", "Container ID", ParameterType.STRING, true));
        columns.add(new TableColumn("image", "Image", ParameterType.STRING, false));
        columns.add(new TableColumn("command", "Command", ParameterType.STRING, false));
        columns.add(new TableColumn("created", "Created", ParameterType.INT, false));
        columns.add(new TableColumn("state", "State", ParameterType.STRING, false));
        columns.add(new TableColumn("name", "Name", ParameterType.STRING, false));
        return columns.toArray(new TableColumn[]{});
    }

    @Override
    public String[][] getValue(String param) throws Exception {
        List<String[]> rows = new ArrayList<>();
        Iterator<Container> containers = docker.containers().all();
        while (containers.hasNext()) {
            Container container = containers.next();
            List<String> row = new ArrayList<>();
            row.add(container.containerId());
            row.add(container.getString("Image"));
            row.add(container.getString("Command"));
            row.add(String.valueOf(container.getInt("Created")));
            row.add(container.getString("State"));
            row.add(container.getJsonArray("Names").getString(0));

            rows.add(row.toArray(new String[]{}));
        }
        return rows.toArray(new String[][]{});
    }

    @Override
    public String getName() {
        return "Docker.Containers";
    }

    @Override
    public String getDescription() {
        return "";
    }
}
