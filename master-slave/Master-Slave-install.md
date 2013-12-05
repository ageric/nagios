##Master-Slave

Master-Slave Mode can still work under one nagios server down  
if nagios master-service down.nagios slave-service will start monitor job and stop them when the master-service recoveries.

####how to install
	you need a nagios slave-service .and copy config file form master-service

	in master-server.you should install nrpe-addon and copy check_nagios.sh file to you nagios's libexec directory.like /usr/local/nagios/libexec

	confirm slave can visit master nrpe daemon

	in slave-server.you also need install nrpe-addon.

	copy check_master_nagios.sh,disable_notifications,enable_notifications,disable_active_service_checks and enable_active_service_checks to libexec directory.

	create crontab job like this:
		*/1 * * * * /usr/local/nagios/libexec/check_master_nagios.sh


