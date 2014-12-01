package com.radensolutions.reporting.custom;

import net.sf.jasperreports.engine.JRDataset;
import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRField;
import net.sf.jasperreports.engine.JRValueParameter;
import org.netxms.api.client.NetXMSClientException;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.constants.AggregationFunction;
import org.netxms.client.datacollection.DciSummaryTableColumn;

import java.io.IOException;
import java.util.*;

public class PerformanceDataSource extends NXCLDataSource {

    private final Type type;
    private List<ReportRow> rows = new LinkedList<ReportRow>();
    private ReportRow currentRow;

    public PerformanceDataSource(Type type, JRDataset dataset, Map<String, ? extends JRValueParameter> parameters) {
        super(dataset, parameters);
        this.type = type;
        this.dataset = dataset;
        this.parameters = parameters;
    }

    @Override
    public void loadData(NXCSession session) throws IOException, NetXMSClientException {
        // TODO: add validation
        Long baseObjectId = (Long) getParameterValue("OBJECT_ID");
        Object fromObj = getParameterValue("PERIOD_START");
        Object toObj = getParameterValue("PERIOD_END");

        Date from;
        Date to;
        if (fromObj instanceof Long && toObj instanceof Long) {
            from = new Date((Long) fromObj * 1000);
            to = new Date((Long) toObj * 1000);
        } else {
            // will produce empty report, but do not crash
            from = new Date(0);
            to = new Date(0);
        }

        ArrayList<DciSummaryTableColumn> columns = new ArrayList<DciSummaryTableColumn>();
        switch (type) {
            case CPU:
            case TRAFFIC:
                columns.add(new DciSummaryTableColumn("System.CPU.Usage", "System.CPU.Usage", 0));
                break;
        }
        Table table = session.queryAdHocDciSummaryTable(baseObjectId, columns, AggregationFunction.AVERAGE, from, to);
        for (TableRow row : table.getAllRows()) {
            rows.add(new ReportRow(row));
        }
        Collections.sort(rows, new Comparator<ReportRow>() {
            @Override
            public int compare(ReportRow o1, ReportRow o2) {
                int avg = o2.average.compareTo(o1.average);
                if (avg != 0) {
                    return avg;
                }
                return o1.nodeName.compareTo(o2.nodeName);
            }
        });
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
        if (name.equalsIgnoreCase("node")) {
            return currentRow.nodeName;
        } else if (name.equalsIgnoreCase("id")) {
            return currentRow.id;
        } else if (name.equalsIgnoreCase("average")) {
            return currentRow.average;
        }
        return null;
    }

    public enum Type {
        CPU,
        TRAFFIC
    }

    private class ReportRow {
        long id;
        String nodeName;
        Double average;

        public ReportRow(long id, String nodeName, Double average) {
            this.id = id;
            this.nodeName = nodeName;
            this.average = average;
        }

        public ReportRow(TableRow row) {
            id = row.getObjectId();
            nodeName = row.get(0).getValue();
            try {
                average = new Double(row.get(1).getValue());
            } catch (NumberFormatException e) {
                average = 0.0;
            }
        }
    }
}
