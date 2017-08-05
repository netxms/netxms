package com.rfelements;

import com.rfelements.model.json.ubiquiti.Ubiquiti;
import org.netxms.agent.Parameter;
import org.netxms.agent.ParameterType;
import org.netxms.bridge.Config;

/**
 * @author Pichanič Ján
 */
public class UbiquitiClientCollector extends AbstractCollector {

    public UbiquitiClientCollector(Config config) {
        super(config, DeviceType.UBIQUITI_CLIENT);
        super.basePath = "/ubiquiti-client/";
        super.protocol = Protocol.HTTPS;
        super.config = config;
    }

    @Override
    public String getName() {
        return "ubiquiti-client";
    }

    @Override
    public void init(Config config) {
    }

    @Override
    public void shutdown() {
        super.shutdown();
    }

    private int[] getSignals(Ubiquiti json) {
        Integer signal = json.getWireless().getSignal();
        Integer rssi = json.getWireless().getRssi();
        Integer chwidth = json.getWireless().getChwidth();
        Integer rxchainmask = json.getWireless().getRxChainmask();
        Integer[] chainrssi = json.getWireless().getChainrssi();
        return computeSignalLevel(signal, rssi, chwidth, rxchainmask, chainrssi);
    }

    private int[] computeSignalLevel(Integer signal, Integer rssi, Integer chwidth, Integer rxchainmask, Integer[] chainrssi) {

        int[] result = new int[2];
        int d = signal - rssi;
        if (chwidth != null) {

            for (int e = 0; e < 3; e++) {
                int p = rxchainmask & (1 << e);
                if (p > 0) {
                    result[e] = rssiToSignal(chainrssi[e], d);
                }
            }
        }
        return result;
    }

    private int rssiToSignal(int a, int b) {
        return b + a;
    }

    @Override
    public Parameter[] getParameters() {
        return new Parameter[]{new Parameter() {

            @Override
            public String getName() {
                return "/ubiquiti-client/uptime(*)";
            }

            @Override
            public String getDescription() {
                return "Device uptime";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.STRING;
            }

            @Override
            public String getValue(String param) throws Exception {
                Ubiquiti ubnt;
                ubnt = getUbntObject(param);
                return String.valueOf(ubnt.getHost().getUptime());
            }

        }, new Parameter() {

            @Override
            public String getName() {
                return "/ubiquiti-client/wireless/airmax_capacity(*)";
            }

            @Override
            public String getDescription() {
                return "Wireless airmax capacity";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.INT;
            }

            @Override
            public String getValue(String param) throws Exception {
                Ubiquiti ubnt;
                ubnt = getUbntObject(param);
                return String.valueOf(ubnt.getWireless().getPolling().getCapacity());
            }
        }, new Parameter() {

            @Override
            public String getName() {
                return "/ubiquiti-client/wireless/airmax_quality(*)";
            }

            @Override
            public String getDescription() {
                return "Wireless airmax quality";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.INT;
            }

            @Override
            public String getValue(String param) throws Exception {
                Ubiquiti ubnt;
                ubnt = getUbntObject(param);
                return String.valueOf(ubnt.getWireless().getPolling().getQuality());
            }
        }, new Parameter() {

            @Override
            public String getName() {
                return "/ubiquiti-client/wireless/noise_floor(*)";
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
                Ubiquiti ubnt;
                ubnt = getUbntObject(param);
                return String.valueOf(ubnt.getWireless().getNoisef());
            }
        }, new Parameter() {

            @Override
            public String getName() {
                return "/ubiquiti-client/wireless/signal_chain0(*)";
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
                Ubiquiti ubnt;
                ubnt = getUbntObject(param);
                return String.valueOf(getSignals(ubnt)[0]);
            }
        }, new Parameter() {

            @Override
            public String getName() {
                return "/ubiquiti-client/wireless/signal_chain1(*)";
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
                Ubiquiti ubnt;
                ubnt = getUbntObject(param);
                return String.valueOf(getSignals(ubnt)[1]);
            }
        }, new Parameter() {

            @Override
            public String getName() {
                return "/ubiquiti-client/wireless/transmit_ccq(*)";
            }

            @Override
            public String getDescription() {
                return "Wireless transmit ccq";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.INT;
            }

            @Override
            public String getValue(String param) throws Exception {
                Ubiquiti ubnt;
                ubnt = getUbntObject(param);
                return String.valueOf(ubnt.getWireless().getCcq());
            }
        }};
    }
}



