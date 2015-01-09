package com.radensolutions.reporting.custom;

import net.sf.jasperreports.engine.JRDataset;
import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRField;
import net.sf.jasperreports.engine.JRValueParameter;
import org.netxms.api.client.NetXMSClientException;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class InventoryDataSource extends NXCLDataSource {

    private List<Row> rows = new ArrayList<Row>();
    private Row currentRow;

    protected InventoryDataSource(JRDataset dataset, Map<String, ? extends JRValueParameter> parameters) {
        super(dataset, parameters);
    }

    @Override
    public void loadData(NXCSession session) throws IOException, NetXMSClientException {
        EntireNetwork entireNetwork = session.findObjectById(1, EntireNetwork.class);
        Set<AbstractObject> nodes = entireNetwork.getAllChilds(GenericObject.OBJECT_NODE);
        for (AbstractObject abstractObject : nodes) {
            Node node = (Node) abstractObject;
            Row row = new Row();
            row.id = (int) node.getObjectId();
            row.name = node.getObjectName();
            row.uname = node.getSystemDescription();
            row.primaryIp = node.getPrimaryIP().getHostName();
            row.bootTime = (int) (node.getBootTime().getTime() / 1000);
            row.lastUpdate = 0;
        }
    }

    @Override
    public boolean next() throws JRException {
        if (rows.isEmpty()) {
            return false;
        }
        currentRow = rows.remove(0);
        return true;
    }

    @Override
    public Object getFieldValue(JRField jrField) throws JRException {
        String name = jrField.getName();
        if ("id".equalsIgnoreCase(name)) {
            return currentRow.id;
        } else if ("name".equalsIgnoreCase(name)) {
            return currentRow.name;
        } else if ("uname".equalsIgnoreCase(name)) {
            return currentRow.uname;
        } else if ("primary_ip".equalsIgnoreCase(name)) {
            return currentRow.primaryIp;
        } else if ("boot_time".equalsIgnoreCase(name)) {
            return currentRow.bootTime;
        } else if ("last_update".equalsIgnoreCase(name)) {
            return currentRow.lastUpdate;
        }
        return null;
    }

    private class Row {
        int id;
        String name;
        String uname;
        String primaryIp;
        int bootTime;
        int lastUpdate;
    }
}
