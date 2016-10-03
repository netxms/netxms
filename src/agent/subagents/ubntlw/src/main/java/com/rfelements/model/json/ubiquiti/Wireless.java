package com.rfelements.model.json.ubiquiti;

/**
 * @author Pichanič Ján
 */
public class Wireless {

    private String mode;

    private String essid;

    private Integer hide_essid;

    private String apmac;

    private Integer countrycode;

    private Integer channel;

    private String frequency;

    private String dfs;

    private String opmode;

    private String antenna;

    private String chains;

    private Integer signal;

    private Integer rssi;

    private Integer noisef;

    private Integer ack;

    private Integer distance;

    private Integer ccq;

    private String txrate;

    private String rxrate;

    private String security;

    private String qos;

    private Integer rstatus;

    private Integer count;

    private Polling polling;

    private Stats stats;

    private Integer wds;

    private Integer aprepeater;

    private Integer chwidth;

    private Integer chanbw;

    private Integer cwmmode;

    private Integer rx_chainmask;

    private Integer tx_chainmask;

    private Integer[] chainrssi;

    private Integer[] chainrssimgmt;

    private Integer[] chainrssiext;

    public String getMode() {
        return mode;
    }

    public void setMode(String mode) {
        this.mode = mode;
    }

    public String getEssid() {
        return essid;
    }

    public void setEssid(String essid) {
        this.essid = essid;
    }

    public Integer getHide_essid() {
        return hide_essid;
    }

    public void setHide_essid(Integer hide_essid) {
        this.hide_essid = hide_essid;
    }

    public String getApmac() {
        return apmac;
    }

    public void setApmac(String apmac) {
        this.apmac = apmac;
    }

    public Integer getCountrycode() {
        return countrycode;
    }

    public void setCountrycode(Integer countrycode) {
        this.countrycode = countrycode;
    }

    public Integer getChannel() {
        return channel;
    }

    public void setChannel(Integer channel) {
        this.channel = channel;
    }

    public String getFrequency() {
        return frequency;
    }

    public void setFrequency(String frequency) {
        this.frequency = frequency;
    }

    public String getDfs() {
        return dfs;
    }

    public void setDfs(String dfs) {
        this.dfs = dfs;
    }

    public String getOpmode() {
        return opmode;
    }

    public void setOpmode(String opmode) {
        this.opmode = opmode;
    }

    public String getAntenna() {
        return antenna;
    }

    public void setAntenna(String antenna) {
        this.antenna = antenna;
    }

    public String getChains() {
        return chains;
    }

    public void setChains(String chains) {
        this.chains = chains;
    }

    public Integer getSignal() {
        return signal;
    }

    public void setSignal(Integer signal) {
        this.signal = signal;
    }

    public Integer getRssi() {
        return rssi;
    }

    public void setRssi(Integer rssi) {
        this.rssi = rssi;
    }

    public Integer getNoisef() {
        return noisef;
    }

    public void setNoisef(Integer noisef) {
        this.noisef = noisef;
    }

    public Integer getAck() {
        return ack;
    }

    public void setAck(Integer ack) {
        this.ack = ack;
    }

    public Integer getDistance() {
        return distance;
    }

    public void setDistance(Integer distance) {
        this.distance = distance;
    }

    public Integer getCcq() {
        return ccq;
    }

    public void setCcq(Integer ccq) {
        this.ccq = ccq;
    }

    public String getTxrate() {
        return txrate;
    }

    public void setTxrate(String txrate) {
        this.txrate = txrate;
    }

    public String getRxrate() {
        return rxrate;
    }

    public void setRxrate(String rxrate) {
        this.rxrate = rxrate;
    }

    public String getSecurity() {
        return security;
    }

    public void setSecurity(String security) {
        this.security = security;
    }

    public String getQos() {
        return qos;
    }

    public void setQos(String qos) {
        this.qos = qos;
    }

    public Integer getRstatus() {
        return rstatus;
    }

    public void setRstatus(Integer rstatus) {
        this.rstatus = rstatus;
    }

    public Integer getCount() {
        return count;
    }

    public void setCount(Integer count) {
        this.count = count;
    }

    public Polling getPolling() {
        return polling;
    }

    public void setPolling(Polling polling) {
        this.polling = polling;
    }

    public Stats getStats() {
        return stats;
    }

    public void setStats(Stats stats) {
        this.stats = stats;
    }

    public Integer getWds() {
        return wds;
    }

    public void setWds(Integer wds) {
        this.wds = wds;
    }

    public Integer getAprepeater() {
        return aprepeater;
    }

    public void setAprepeater(Integer aprepeater) {
        this.aprepeater = aprepeater;
    }

    public Integer getChwidth() {
        return chwidth;
    }

    public void setChwidth(Integer chwidth) {
        this.chwidth = chwidth;
    }

    public Integer getChanbw() {
        return chanbw;
    }

    public void setChanbw(Integer chanbw) {
        this.chanbw = chanbw;
    }

    public Integer getCwmmode() {
        return cwmmode;
    }

    public void setCwmmode(Integer cwmmode) {
        this.cwmmode = cwmmode;
    }

    public Integer getRxChainmask() {
        return rx_chainmask;
    }

    public void setRx_chainmask(Integer rx_chainmask) {
        this.rx_chainmask = rx_chainmask;
    }

    public Integer getTx_chainmask() {
        return tx_chainmask;
    }

    public void setTx_chainmask(Integer tx_chainmask) {
        this.tx_chainmask = tx_chainmask;
    }

    public Integer[] getChainrssi() {
        return chainrssi;
    }

    public void setChainrssi(Integer[] chainrssi) {
        this.chainrssi = chainrssi;
    }

    public Integer[] getChainrssimgmt() {
        return chainrssimgmt;
    }

    public void setChainrssimgmt(Integer[] chainrssimgmt) {
        this.chainrssimgmt = chainrssimgmt;
    }

    public Integer[] getChainrssiext() {
        return chainrssiext;
    }

    public void setChainrssiext(Integer[] chainrssiext) {
        this.chainrssiext = chainrssiext;
    }
}
