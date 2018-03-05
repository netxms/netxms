package com.github.tomaskir.netxms.subagents.bind9;

import java.util.ArrayList;
import java.util.List;

/**
 * Contains a {@code List<String[]>} of all parameters provided by this Subagent Plugin.<br><br>
 * Each entry in the list has the following format:<br>
 * entry[0] - Parameter name<br>
 * entry[1] - Parameter description<br>
 * entry[2] - name in bind9 statistics file<br>
 *
 * @author Tomas Kirnak
 * @since 2.1-M1
 */
public final class Parameters {

    private final List<String[]> list = new ArrayList<>();

    // getter
    public List<String[]> getList() {
        return list;
    }

    // constructor
    Parameters() {
        list.add(new String[]{
                "bind9.Requests.Received.IPv4",
                "Received IPv4 requests since bind9 started",
                "IPv4 requests received"
        });
        list.add(new String[]{
                "bind9.Requests.Received.IPv6",
                "Received IPv6 requests since bind9 started",
                "IPv6 requests received"
        });
        list.add(new String[]{
                "bind9.Requests.Recursive.Rejected",
                "Rejected recursive queries since bind9 started",
                "recursive queries rejected"
        });
        list.add(new String[]{
                "bind9.Answers.Auth",
                "Queries that resulted in an authoritative answer since bind9 started",
                "queries resulted in authoritative answer"
        });
        list.add(new String[]{
                "bind9.Answers.NonAuth",
                "Queries that resulted in a non-authoritative answer since bind9 started",
                "queries resulted in non authoritative answer"
        });
        list.add(new String[]{
                "bind9.Answers.nxrrset",
                "Queries that resulted in a nxrrset answer since bind9 started",
                "queries resulted in nxrrset"
        });
        list.add(new String[]{
                "bind9.Answers.SERVFAIL",
                "Queries that resulted in a SERVFAIL answer since bind9 started",
                "queries resulted in SERVFAIL"
        });
        list.add(new String[]{
                "bind9.Answers.NXDOMAIN",
                "Queries that resulted in a NXDOMAIN answer since bind9 started",
                "queries resulted in NXDOMAIN"
        });
        list.add(new String[]{
                "bind9.Answers.Recursive",
                "Queries that caused recursion since bind9 started",
                "queries caused recursion"
        });
        list.add(new String[]{
                "bind9.Requests.Failed.Other",
                "Other query failures since bind9 started",
                "other query failures"
        });
        list.add(new String[]{
                "bind9.Notifies.Sent.IPv4",
                "Sent IPv4 notifies since bind9 started",
                "IPv4 notifies sent"
        });
        list.add(new String[]{
                "bind9.Notifies.Sent.IPv6",
                "Sent IPv6 notifies since bind9 started",
                "IPv6 notifies sent"
        });
        list.add(new String[]{
                "bind9.Notifies.Received.IPv4",
                "Received IPv4 notifies since bind9 started",
                "IPv4 notifies received"
        });
        list.add(new String[]{
                "bind9.Notifies.Received.IPv6",
                "Received IPv6 notifies since bind9 started",
                "IPv6 notifies received"
        });
    }

}
