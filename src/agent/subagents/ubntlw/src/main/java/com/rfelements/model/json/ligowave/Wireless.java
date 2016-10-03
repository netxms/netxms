package com.rfelements.model.json.ligowave;

/**
 * @author Pichanič Ján
 */
public class Wireless {

    private Integer rxRate;

    private Double rxBytes;

    private String channelWidth;

    private Integer[] signal;

    private Integer[] noise;

    private Integer txRate;

    private Integer txPackets;

    private Integer txRetryPercent;

    private Integer rxPackets;

    private String radioMode;

    private String mcs;

    private Integer txPower;

    public Integer getRxRate() {
        return rxRate;
    }

    public void setRxRate(Integer rxRate) {
        this.rxRate = rxRate;
    }

    public Double getRxBytes() {
        return rxBytes;
    }

    public void setRxBytes(Double rxBytes) {
        this.rxBytes = rxBytes;
    }

    public String getChannelWidth() {
        return channelWidth;
    }

    public void setChannelWidth(String channelWidth) {
        this.channelWidth = channelWidth;
    }

    public Integer[] getSignal() {
        return signal;
    }

    public void setSignal(Integer[] signal) {
        this.signal = signal;
    }

    public Integer[] getNoise() {
        return noise;
    }

    public void setNoise(Integer[] noise) {
        this.noise = noise;
    }

    public Integer getTxRate() {
        return txRate;
    }

    public void setTxRate(Integer txRate) {
        this.txRate = txRate;
    }

    public Integer getTxPackets() {
        return txPackets;
    }

    public void setTxPackets(Integer txPackets) {
        this.txPackets = txPackets;
    }

    public Integer getTxRetryPercent() {
        return txRetryPercent;
    }

    public void setTxRetryPercent(Integer txRetryPercent) {
        this.txRetryPercent = txRetryPercent;
    }

    public Integer getRxPackets() {
        return rxPackets;
    }

    public void setRxPackets(Integer rxPackets) {
        this.rxPackets = rxPackets;
    }

    public String getRadioMode() {
        return radioMode;
    }

    public void setRadioMode(String radioMode) {
        this.radioMode = radioMode;
    }

    public String getMcs() {
        return mcs;
    }

    public void setMcs(String mcs) {
        this.mcs = mcs;
    }

    public Integer getTxPower() {
        return txPower;
    }

    public void setTxPower(Integer txPower) {
        this.txPower = txPower;
    }
}
