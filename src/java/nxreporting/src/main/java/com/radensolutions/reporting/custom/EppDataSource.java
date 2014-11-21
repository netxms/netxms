package com.radensolutions.reporting.custom;

import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRField;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static org.netxms.client.events.EventProcessingPolicyRule.*;

public class EppDataSource extends NXCLDataSource {

    private static final Logger log = LoggerFactory.getLogger(EppDataSource.class);

    private final Map<Long, ServerAction> actionMap = new HashMap<Long, ServerAction>();

    private List<Row> rows = new ArrayList<Row>();
    private Row currentRow;

    protected EppDataSource() {
        super(null, null);
    }

    @Override
    public void loadData(NXCSession session) throws IOException, NXCException {
        EventProcessingPolicy epp = session.getEventProcessingPolicy();

        for (ServerAction action : session.getActions()) {
            actionMap.put(action.getId(), action);
        }

        int ruleIndex = 0;
        for (EventProcessingPolicyRule rule : epp.getRules()) {
            List<Cell> filterRows = new ArrayList<Cell>();
            List<Cell> actionRows = new ArrayList<Cell>();

            processSources(session, rule, filterRows);
            processEvents(session, rule, filterRows);
            processAlarms(rule, actionRows);
            processStop(rule, actionRows);

            int count = Math.max(filterRows.size(), actionRows.size());
            String name = rule.getComments();
            if (rule.isDisabled()) {
                name += " (disabled)";
            }
            for (int i = 0; i < count; i++) {
                rows.add(new Row(ruleIndex, name, safePop(filterRows), safePop(actionRows)));
            }
            ruleIndex++;
        }
    }

    private void processStop(EventProcessingPolicyRule rule, List<Cell> actionRows) {
        int flags = rule.getFlags();
        if ((flags & STOP_PROCESSING) == STOP_PROCESSING) {
            actionRows.add(new Cell("Stop Processing", true));
        }
    }

    private void processAlarms(EventProcessingPolicyRule rule, List<Cell> actionRows) {
        int flags = rule.getFlags();
        if ((flags & GENERATE_ALARM) == GENERATE_ALARM) {
            switch (rule.getAlarmSeverity()) {
                case NORMAL:
                case WARNING:
                case MINOR:
                case MAJOR:
                case CRITICAL:
                case UNKNOWN:
                    actionRows.add(new Cell("Generate alarm", true));
                    actionRows.add(new Cell(rule.getAlarmMessage()));
                    actionRows.add(new Cell("with key \"" + rule.getAlarmKey() + "\""));
                    break;
                case TERMINATE:
                    actionRows.add(new Cell("Terminate alarm", true));
                    actionRows.add(new Cell("with key \"" + rule.getAlarmKey() + "\""));
                    break;
                case RESOLVE:
                    actionRows.add(new Cell("Resolve alarm", true));
                    actionRows.add(new Cell("with key \"" + rule.getAlarmKey() + "\""));
                    break;
            }
        }

        List<Long> actions = rule.getActions();
        if (actions.size() > 0) {
            actionRows.add(new Cell("Execute actions", true));
            for (Long actionId : actions) {
                ServerAction action = actionMap.get(actionId);
                actionRows.add(new Cell(action.getName()));
            }
        }
    }

    private void processEvents(NXCSession session, EventProcessingPolicyRule rule, List<Cell> filterRows) {
        int flags = rule.getFlags();

        List<Long> events = rule.getEvents();
        if (events.size() > 0) {
            String prefix = "";
            if (!filterRows.isEmpty()) {
                prefix = "AND ";
            } else {
                prefix = "IF ";
            }
            if (rule.isEventsInverted()) {
                filterRows.add(new Cell(prefix + "event is NOT", true));
            } else {
                filterRows.add(new Cell(prefix + "event is", true));
            }
            for (Long eventId : events) {
                EventTemplate event = session.findEventTemplateByCode(eventId);
                filterRows.add(new Cell(event.getName()));
            }
        }

        if ((flags & SEVERITY_ANY) != SEVERITY_ANY) {
            String prefix = "";
            if (!filterRows.isEmpty()) {
                prefix = "AND ";
            } else {
                prefix = "IF ";
            }
            filterRows.add(new Cell(prefix + "event severity is", true));
            if ((flags & SEVERITY_NORMAL) == SEVERITY_NORMAL) {
                filterRows.add(new Cell("Normal"));
            }
            if ((flags & SEVERITY_WARNING) == SEVERITY_WARNING) {
                filterRows.add(new Cell("Warning"));
            }
            if ((flags & SEVERITY_MINOR) == SEVERITY_MINOR) {
                filterRows.add(new Cell("Minor"));
            }
            if ((flags & SEVERITY_MAJOR) == SEVERITY_MAJOR) {
                filterRows.add(new Cell("Major"));
            }
            if ((flags & SEVERITY_CRITICAL) == SEVERITY_CRITICAL) {
                filterRows.add(new Cell("Critical"));
            }
        }
    }

    private void processSources(NXCSession session, EventProcessingPolicyRule rule, List<Cell> filterRows) {
        List<Long> sources = rule.getSources();
        if (sources.size() > 0) {
            if (rule.isSourceInverted()) {
                filterRows.add(new Cell("IF source object is NOT", true));
            } else {
                filterRows.add(new Cell("IF source object is", true));
            }

            for (Long objectId : sources) {
                AbstractObject object = session.findObjectById(objectId);
                final String objectName;
                if (object != null) {
                    objectName = object.getObjectName();
                } else {
                    objectName = "[" + objectId + "]";
                }
                filterRows.add(new Cell(objectName));
            }
        }
    }

    private Cell safePop(List<Cell> rows) {
        final Cell cell;
        if (!rows.isEmpty()) {
            cell = rows.remove(0);
        } else {
            cell = null;
        }
        return cell;
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
            return currentRow.index;
        } else if ("name".equalsIgnoreCase(name)) {
            return currentRow.name;
        } else if ("filter".equalsIgnoreCase(name)) {
            return currentRow.filter.text;
        } else if ("filterHighlight".equalsIgnoreCase(name)) {
            return currentRow.filter.highlight;
        } else if ("action".equalsIgnoreCase(name)) {
            return currentRow.action.text;
        } else if ("actionHighlight".equalsIgnoreCase(name)) {
            return currentRow.action.highlight;
        }
        return null;
    }

    private class Cell {
        private final String text;
        private final boolean highlight;

        public Cell(String text) {
            this(text, false);
        }

        public Cell(String text, boolean highlight) {
            this.text = text;
            this.highlight = highlight;
        }
    }

    private class Row {
        private final int index;
        private final String name;
        private final Cell filter;
        private final Cell action;

        public Row(int index, String name, Cell filter, Cell action) {
            this.index = index;
            this.name = name;
            this.filter = filter == null ? new Cell("") : filter;
            this.action = action == null ? new Cell("") : action;
        }
    }
}
