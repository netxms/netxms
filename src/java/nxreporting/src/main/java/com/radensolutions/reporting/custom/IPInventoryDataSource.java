package com.radensolutions.reporting.custom;

import net.sf.jasperreports.engine.JRDataset;
import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRField;
import net.sf.jasperreports.engine.JRValueParameter;
import org.netxms.api.client.NetXMSClientException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Subnet;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class IPInventoryDataSource extends NXCLDataSource {

    private Subnet subnet;
    private ArrayList<Row> rows = new ArrayList<Row>();
    private Row currentRow;

    protected IPInventoryDataSource(JRDataset dataset, Map<String, ? extends JRValueParameter> parameters) {
        super(dataset, parameters);
    }

    @Override
    public void loadData(NXCSession session) throws IOException, NetXMSClientException {
        Long subnetId = (Long) getParameterValue("OBJECT_ID");
        subnet = session.findObjectById(subnetId, Subnet.class);
        if (subnet != null) {
            long[] map = session.getSubnetAddressMap(subnetId);
            List<AbstractObject> objects = session.findMultipleObjects(map, true);
            for (int i = 0; i < map.length; i++) {
                rows.add(new Row(i, map[i], objects.get(i)));
            }
        }
    }

    @Override
    public boolean next() throws JRException {
        if (!rows.isEmpty()) {
            currentRow = rows.remove(0);
            return true;
        }
        return false;
    }

    @Override
    public Object getFieldValue(JRField jrField) throws JRException {
        String name = jrField.getName();
        if (name.equalsIgnoreCase("ip")) {
            if (subnet.getMaskBits() < 24) {
                return String.format(".%d.%d", (currentRow.index & 0xFF00) >> 8, currentRow.index & 0xFF);
            }
            return String.format(".%d", currentRow.index & 0xFF);
        } else if (name.equalsIgnoreCase("status")) {
            return currentRow.status;
        } else if (name.equalsIgnoreCase("name")) {
            return currentRow.name;
        } else if (name.equalsIgnoreCase("subnet")) {
            return String.format("%s/%d", subnet.getPrimaryIP().getHostAddress(), subnet.getMaskBits());
        }
        return null;
    }

    private class Row {
        int index;
        int status;
        String name;

        public Row(int index, Long id, AbstractObject object) {
            this.index = index;
            if (id == 0L) {
                status = 0; // free
            } else if (id == 0xFFFFFFFFL) {
                status = 1; // reserved
            } else {
                status = 2; // used
            }
            this.name = object.getStatus() != ObjectStatus.UNKNOWN ? object.getObjectName() : null;
        }
    }
}
