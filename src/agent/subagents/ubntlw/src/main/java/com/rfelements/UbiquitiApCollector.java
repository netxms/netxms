package com.rfelements;

import com.rfelements.model.json.ubiquiti.Ubiquiti;
import org.netxms.agent.Parameter;
import org.netxms.agent.ParameterType;
import org.netxms.bridge.Config;

/**
 * @author Pichanič Ján
 */
public class UbiquitiApCollector extends AbstractCollector {

    public UbiquitiApCollector(Config config) {
        super(config, DeviceType.UBIQUITI_AP);
        super.basePath = "/ubiquiti-ap/";
        super.protocol = Protocol.HTTPS;
        super.config = config;
    }

    @Override
    public String getName() {
        return "ubiquiti-ap";
    }

    @Override
    public void init(Config config) {
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
                return "/ubiquiti-ap/uptime(*)";
            }

            @Override
            public String getDescription() {
                return "Device uptime";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.INT;
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
                return "/ubiquiti-ap/wireless/airmax_capacity(*)";
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
                return "/ubiquiti-ap/wireless/airmax_quality(*)";
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
                return "/ubiquiti-ap/wireless/noise_floor(*)";
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
                return "/ubiquiti-ap/wireless/frequency(*)";
            }

            @Override
            public String getDescription() {
                return "Wireless frequency";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.INT;
            }

            @Override
            public String getValue(String param) throws Exception {
                Ubiquiti ubnt;
                ubnt = getUbntObject(param);
                String frequency = ubnt.getWireless().getFrequency();
                String[] split = frequency.split("\\s+");
                return split[0].trim();
            }
        }, new Parameter() {

            @Override
            public String getName() {
                return "/ubiquiti-ap/wireless/channel_width(*)";
            }

            @Override
            public String getDescription() {
                return "Wireless channel width";
            }

            @Override
            public ParameterType getType() {
                return ParameterType.INT;
            }

            @Override
            public String getValue(String param) throws Exception {
                Ubiquiti ubnt;
                ubnt = getUbntObject(param);
                return String.valueOf(ubnt.getWireless().getChwidth());
            }
        }, new Parameter() {

            @Override
            public String getName() {
                return "/ubiquiti-ap/wireless/transmit_ccq(*)";
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
