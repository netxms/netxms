package org.netxms.report;

import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JasperExportManager;

public class Exporter {
    public static void main(final String[] args) {
        int ret = 0;
        if (args.length == 2) {
            //noinspection CatchGenericClass
            try {
                if (!export(args[0], args[1])) {
                    ret = 3;
                }
            } catch (Exception e) {
                System.out.println("ERROR: " + e.getMessage());
                ret = 2;
            }
        } else {
            help();
            ret = 1;
        }

        //noinspection CallToSystemExit
        System.exit(ret);
    }

    private static boolean export(final String reportFileName, final String outputFileName) throws JRException {
        boolean ret = false;
        if (outputFileName.endsWith(".pdf")) {
            JasperExportManager.exportReportToPdfFile(reportFileName, outputFileName);
            ret = true;
        }

        return ret;
    }

    private static void help() {
        System.out.println("Usage: <input file> <output file.pdf>");
    }

}
