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
                columns.add(new DciSummaryTableColumn("System.CPU.Usage", "System.CPU.Usage", 0));
                break;
            case TRAFFIC:
                columns.add(new DciSummaryTableColumn("System.CPU.Usage", "System.CPU.Usage", 0));
                break;
            case DISK:
                columns.add(new DciSummaryTableColumn("FileSystem.UsedPerc", "FileSystem\\.UsedPerc.*", DciSummaryTableColumn.REGEXP_MATCH));
                break;
        }
        Table table = session.queryAdHocDciSummaryTable(baseObjectId, columns, AggregationFunction.AVERAGE, from, to, true);
        for (TableRow row : table.getAllRows()) {
            rows.add(new ReportRow(row));
        }
        switch (type) {
            case CPU:
                Collections.sort(rows, new Comparator<ReportRow>() {
                    @Override
                    public int compare(ReportRow o1, ReportRow o2) {
                        int valueResult = o2.value.compareTo(o1.value);
                        if (valueResult != 0) {
                            return valueResult;
                        }
                        return o1.nodeName.compareTo(o2.nodeName);
                    }
                });
                break;
            case DISK:
                Collections.sort(rows, new Comparator<ReportRow>() {
                    @Override
                    public int compare(ReportRow o1, ReportRow o2) {
                        int nameResult = o1.nodeName.compareTo(o2.nodeName);
                        if (nameResult == 0) {
                            return o2.value.compareTo(o1.value);
                        }
                        return nameResult;
                    }
                });
                break;
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
        if (name.equalsIgnoreCase("node")) {
            return currentRow.nodeName;
        } else if (name.equalsIgnoreCase("id")) {
            return currentRow.id;
        } else if (name.equalsIgnoreCase("instance")) {
            return currentRow.instance;
        } else if (name.equalsIgnoreCase("value")) {
            return currentRow.value;
        }
        return null;
    }

    public enum Type {
        CPU,
        TRAFFIC,
        DISK
    }

    private class ReportRow {
        long id;
        String nodeName;
        String instance;
        Double value;

        public ReportRow(long id, String nodeName, String instance, Double value) {
            this.id = id;
            this.nodeName = nodeName;
            this.instance = instance;
            this.value = value;
        }

        public ReportRow(TableRow row) {
            id = row.getObjectId();
            nodeName = row.get(0).getValue();
            instance = row.get(1).getValue();
            try {
                value = new Double(row.get(2).getValue());
            } catch (NumberFormatException e) {
                value = 0.0;
            }
        }
    }
}
