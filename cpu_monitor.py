#!/usr/bin/env python
# -*- coding:utf-8 -*-
"""
Author:DAVID.ZHANG
Date:2013/12/5
Function:Monitor CPU Nagios-Plug-in.
"""

import time
from optparse import OptionParser
import sys

OK=0
WARNING=1
CRITICAL=2
Error=3

def opt():
    """user parameters"""

    parser = OptionParser(usage="usage: \n -t sleep time. \n -c critical. \n -w warning.")
    parser.add_option("-t", default=5, action="store", type="int", dest="sleep_time")
    parser.add_option("-c", default="30%", action="store", type="string", dest="Critical")
    parser.add_option("-w", default="20%", action="store", type="string", dest="Warning")
    return parser.parse_args()

def getCpuInfo():
    """ Get CPU information """
    ld = []
    with open('/proc/stat', 'r') as fd:
        for line in fd.read().strip().split('\n'):
            if line.startswith('cpu '):
                ld.append(line)
        return ld[0]

def parserCpu(data):
    """ The current cpu avalability. """

    CPULOG_1 = getCpuInfo()
    SYS_IDLE_1 = CPULOG_1.split()[4]
    Total_1 = int(CPULOG_1.split()[1])+int(CPULOG_1.split()[2])+int(CPULOG_1.split()[3])+int(CPULOG_1.split()[4])+int(CPULOG_1.split()[5])+int(CPULOG_1.split()[6])+int(CPULOG_1.split()[7])

    time.sleep(data.sleep_time)
    
    CPULOG_2 = getCpuInfo()
    SYS_IDLE_2 = CPULOG_2.split()[4]
    Total_2 = int(CPULOG_2.split()[1])+int(CPULOG_2.split()[2])+int(CPULOG_2.split()[3])+int(CPULOG_2.split()[4])+int(CPULOG_2.split()[5])+int(CPULOG_2.split()[6])+int(CPULOG_2.split()[7])
     
    SYS_IDLE = int(SYS_IDLE_2) - int(SYS_IDLE_1)
    Total = int(Total_2) - int(Total_1)
    data = float(SYS_IDLE)/Total*100
    return int(data)   

def main():
    """ The main function """
    opts, args =  opt()
    SYS_USAGE = parserCpu(opts)
    
    if opts.Warning[-1] == '%' and opts.Critical[-1] == '%':
        waring_init = int(opts.Warning.split('%')[0])
        critical_init = int(opts.Critical.split('%')[0])
    else:
        print "Error: format Error.\nformat:int+%s.eg:60%s" % ('%','%')
        sys.exit(Error) 
    
    if SYS_USAGE > waring_init and SYS_USAGE > critical_init:
        print "OK, %s%s" % (SYS_USAGE,'%')
        sys.exit(OK)
    elif SYS_USAGE <= waring_init:
        print "Warning, %s%s" % (SYS_USAGE,'%') 
        sys.exit(WARNING)
    elif SYS_USAGE <= critical_init:
        print "Critical, %s%s" % (SYS_USAGE,'%')
        sys.exit(CRITICAL)
    else:
        print "UNKNOWN, %s%S" % (SYS_USAGE,'%')
 
if __name__ == "__main__":
    main()
