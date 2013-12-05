#!/bin/sh

result=$(/usr/local/nagios/libexec/check_nrpe -H 192.168.80.69 -c check_nagios)
if [ "$num" -eq 1 ];then
  #stop slave moitor
  /usr/local/nagios/libexec/disable_active_service_checks
  /usr/local/nagios/libexec/disable_notifications
else
   #start slave monitor
  /usr/local/nagios/libexec/enable_active_service_checks
  /usr/local/nagios/libexec/enable_notifications
fi
