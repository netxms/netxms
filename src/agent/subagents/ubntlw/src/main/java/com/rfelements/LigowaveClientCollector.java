package com.rfelements;

import com.rfelements.exception.CollectorException;
import com.rfelements.model.json.ligowave.Ligowave;
import org.netxms.agent.Parameter;
import org.netxms.agent.ParameterType;
import org.netxms.bridge.Config;

/**
 * @author Pichanič Ján
 */
public class LigowaveClientCollector extends AbstractCollector {

    public LigowaveClientCollector(Config config) throws CollectorException {
        super(config, DeviceType.LIGOWAVE_CLIENT);
        super.protocol = Protocol.HTTP;
        super.basePath = "/ligowave-client/";
        super.config = config;
    }

    @Override
    public String getName() {
        return "ligowave-client";
    }

    @Override
    public void init(Config config) {
        //		super.config = config;
    }

    @Override
    public void shutdown() {
        super.shutdown();
    }

    @Override
    public Parameter[] getParameters() {
        return new Parameter[]{new Parameter() {

            @Override
            public String getName() {
                return "/ligowave-client/wireless/signal_chain0(*)";
            }

            @Override
            public String getDescription() {
                return "Wireless signal chain 0";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.INT;
            }

            @Override
            public String getValue(String param) throws Exception {
                Ligowave dlb;
                dlb = getDlbObject(param);
                Integer signal = dlb.getWirelessInformation().getPeers()[0].getSignal()[0];
                return String.valueOf(signal);
            }
        }, new Parameter() {

            @Override
            public String getName() {
                return "/ligowave-client/wireless/signal_chain1(*)";
            }

            @Override
            public String getDescription() {
                return "Wireless signal chain 1";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.INT;
            }

            @Override
            public String getValue(String param) throws Exception {
                Ligowave dlb;
                dlb = getDlbObject(param);
                Integer signal = dlb.getWirelessInformation().getPeers()[0].getSignal()[1];
                return String.valueOf(signal);
            }
        }, new Parameter() {

            @Override
            public String getName() {
                return "/ligowave-client/wireless/noise_floor(*)";
            }

            @Override
            public String getDescription() {
                return "Wireless noise floor";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.INT;
            }

            @Override
            public String getValue(String param) throws Exception {
                Ligowave dlb;
                dlb = getDlbObject(param);
                Integer[] noise = dlb.getRemoteInformation()[0].getWireless().getNoise();
                return String.valueOf(noise[0]);
            }
        }, new Parameter() {

            @Override
            public String getName() {
                return "/ligowave-client/wireless/rx_ccq_percent(*)";
            }

            @Override
            public String getDescription() {
                return "Wireless rx ccq percent";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.INT;
            }

            @Override
            public String getValue(String param) throws Exception {
                Ligowave dlb;
                dlb = getDlbObject(param);
                Integer rxCcq = dlb.getWirelessInformation().getPeers()[0].getRxCcqPercent();
                return String.valueOf(rxCcq);
            }
        }, new Parameter() {

            @Override
            public String getName() {
                return "/ligowave-client/wireless/rx_rate(*)";
            }

            @Override
            public String getDescription() {
                return "Wireless rx rate";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.INT;
            }

            @Override
            public String getValue(String param) throws Exception {
                Ligowave dlb;
                dlb = getDlbObject(param);
                Integer rxRate = dlb.getWirelessInformation().getPeers()[0].getRxRate();
                return String.valueOf(rxRate);
            }
        }, new Parameter() {

            @Override
            public String getName() {
                return "/ligowave-client/wireless/tx_ccq_percent(*)";
            }

            @Override
            public String getDescription() {
                return "Wireless tx ccq percent";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.INT;
            }

            @Override
            public String getValue(String param) throws Exception {
                Ligowave dlb;
                dlb = getDlbObject(param);
                Integer rxCcq = dlb.getWirelessInformation().getPeers()[0].getTxCcqPercent();
                return String.valueOf(rxCcq);
            }
        }, new Parameter() {

            @Override
            public String getName() {
                return "/ligowave-client/wireless/tx_rate(*)";
            }

            @Override
            public String getDescription() {
                return "Wireless tx rate";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.INT;
            }

            @Override
            public String getValue(String param) throws Exception {
                Ligowave dlb;
                dlb = getDlbObject(param);
                Integer txRate = dlb.getWirelessInformation().getPeers()[0].getTxRate();
                return String.valueOf(txRate);
            }
        }};
    }
}
