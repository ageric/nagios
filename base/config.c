/*****************************************************************************
 *
 * CONFIG.C - Configuration input and verification routines for Nagios
 *
 * License:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *****************************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"
#include "../include/macros.h"
#include "../include/nagios.h"
#include "../include/broker.h"
#include "../include/nebmods.h"
#include "../include/nebmodules.h"


extern char	*log_file;
extern char     *command_file;
extern char     *temp_file;
extern char     *temp_path;
extern char     *check_result_path;
extern char     *lock_file;
extern char	*log_archive_path;

extern char     *nagios_user;
extern char     *nagios_group;

extern char     *macro_user[MAX_USER_MACROS];

extern char     *global_host_event_handler;
extern char     *global_service_event_handler;
extern command  *global_host_event_handler_ptr;
extern command  *global_service_event_handler_ptr;

extern char     *ocsp_command;
extern char     *ochp_command;
extern command  *ocsp_command_ptr;
extern command  *ochp_command_ptr;

extern char     *illegal_object_chars;
extern char     *illegal_output_chars;

extern int      use_regexp_matches;
extern int      use_true_regexp_matching;

extern int	use_syslog;
extern int      log_notifications;
extern int      log_service_retries;
extern int      log_host_retries;
extern int      log_event_handlers;
extern int      log_external_commands;
extern int      log_passive_checks;

extern int      service_check_timeout;
extern int      service_check_timeout_state;
extern int      host_check_timeout;
extern int      event_handler_timeout;
extern int      notification_timeout;
extern int      ocsp_timeout;
extern int      ochp_timeout;

extern int      log_initial_states;

extern int      daemon_mode;
extern int      daemon_dumps_core;

extern int      verify_config;
extern int      verify_object_relationships;
extern int      verify_circular_paths;
extern int      test_scheduling;
extern int      precache_objects;
extern int      use_precached_objects;

extern int      interval_length;
extern int      service_inter_check_delay_method;
extern int      host_inter_check_delay_method;
extern int      service_interleave_factor_method;
extern int      max_host_check_spread;
extern int      max_service_check_spread;

extern sched_info scheduling_info;

extern int      max_child_process_time;

extern int      max_parallel_service_checks;

extern int      check_reaper_interval;
extern int      max_check_reaper_time;
extern int      service_freshness_check_interval;
extern int      host_freshness_check_interval;
extern int      auto_rescheduling_interval;
extern int      auto_rescheduling_window;

extern int      check_external_commands;
extern int      check_orphaned_services;
extern int      check_orphaned_hosts;
extern int      check_service_freshness;
extern int      check_host_freshness;
extern int      auto_reschedule_checks;

extern int      additional_freshness_latency;

extern int      check_for_updates;
extern int      bare_update_check;

extern int      use_aggressive_host_checking;
extern unsigned long cached_host_check_horizon;
extern unsigned long cached_service_check_horizon;
extern int      enable_predictive_host_dependency_checks;
extern int      enable_predictive_service_dependency_checks;

extern int      soft_state_dependencies;

extern int      retain_state_information;
extern int      retention_update_interval;
extern int      use_retained_program_state;
extern int      use_retained_scheduling_info;
extern int      retention_scheduling_horizon;
extern unsigned long retained_host_attribute_mask;
extern unsigned long retained_service_attribute_mask;
extern unsigned long retained_contact_host_attribute_mask;
extern unsigned long retained_contact_service_attribute_mask;
extern unsigned long retained_process_host_attribute_mask;
extern unsigned long retained_process_service_attribute_mask;

extern int      log_rotation_method;

extern int      enable_notifications;
extern int      execute_service_checks;
extern int      accept_passive_service_checks;
extern int      execute_host_checks;
extern int      accept_passive_host_checks;
extern int      enable_event_handlers;
extern int      obsess_over_services;
extern int      obsess_over_hosts;
extern int      enable_failure_prediction;

extern int      translate_passive_host_checks;
extern int      passive_host_checks_are_soft;

extern int      aggregate_status_updates;
extern int      status_update_interval;

extern int      time_change_threshold;

extern unsigned long event_broker_options;

extern int      process_performance_data;

extern int      enable_flap_detection;

extern double   low_service_flap_threshold;
extern double   high_service_flap_threshold;
extern double   low_host_flap_threshold;
extern double   high_host_flap_threshold;

extern int      use_large_installation_tweaks;
extern int      enable_environment_macros;
extern int      free_child_process_memory;
extern int      child_processes_fork_twice;

extern int      date_format;
extern char     *use_timezone;

extern unsigned long    max_check_result_file_age;

extern char             *debug_file;
extern int              debug_level;
extern int              debug_verbosity;
extern unsigned long    max_debug_file_size;

extern int              allow_empty_hostgroup_assignment;

/*** helpers ****/
/*
 * find a command with arguments still attached
 * if we're unsuccessful, the buffer pointed to by 'name' is modified
 * to have only the real command name (everything up until the first '!')
 */
static command *find_bang_command(char *name)
{
	char *bang;
	command *cmd;

	if (!name)
		return NULL;

	bang = strchr(name, '!');
	if (!bang)
		return find_command(name);
	*bang = 0;
	cmd = find_command(name);
	*bang = '!';
	return cmd;
}


/******************************************************************/
/************** CONFIGURATION INPUT FUNCTIONS *********************/
/******************************************************************/

/* read all configuration data */
int read_all_object_data(char *main_config_file) {
	int result = OK;
	int options = 0;
	int cache = FALSE;
	int precache = FALSE;

	options = READ_ALL_OBJECT_DATA;

	/* cache object definitions if we're up and running */
	if(verify_config == FALSE && test_scheduling == FALSE)
		cache = TRUE;

	/* precache object definitions */
	if(precache_objects == TRUE && (verify_config == TRUE || test_scheduling == TRUE))
		precache = TRUE;

	/* read in all host configuration data from external sources */
	result = read_object_config_data(main_config_file, options, cache, precache);
	if(result != OK)
		return ERROR;

	return OK;
	}

static void obsoleted_warning(const char *key, const char *msg)
{
	if (msg)
		logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: %s is deprecated and will be removed. %s\n", key, msg);
	else
		logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: %s is deprecated and will be removed.\n", key);
}

/* process the main configuration file */
int read_main_config_file(char *main_config_file) {
	char *input = NULL;
	char *variable = NULL;
	char *value = NULL;
	char *error_message = NULL;
	char *temp_ptr = NULL;
	mmapfile *thefile = NULL;
	int current_line = 0;
	int error = FALSE;
	char *argptr = NULL;
	DIR *tmpdir = NULL;
	nagios_macros *mac;

	mac = get_global_macros();


	/* open the config file for reading */
	if((thefile = mmap_fopen(main_config_file)) == NULL) {
		logit(NSLOG_CONFIG_ERROR, TRUE, "Error: Cannot open main configuration file '%s' for reading!", main_config_file);
		return ERROR;
		}

	/* save the main config file macro */
	my_free(mac->x[MACRO_MAINCONFIGFILE]);
	if((mac->x[MACRO_MAINCONFIGFILE] = (char *)strdup(main_config_file)))
		strip(mac->x[MACRO_MAINCONFIGFILE]);

	/* process all lines in the config file */
	while(1) {

		/* free memory */
		my_free(input);
		my_free(variable);
		my_free(value);

		/* read the next line */
		if((input = mmap_fgets_multiline(thefile)) == NULL)
			break;

		current_line = thefile->current_line;

		strip(input);

		/* skip blank lines and comments */
		if(input[0] == '\x0' || input[0] == '#')
			continue;

		/* get the variable and value */
		if (!my_str2parts(input,'=', &variable,&value)) {
			asprintf(&error_message, "bad variable declaration: %s", input);
			error = TRUE;
			break;
			}

		strip(variable);
		strip(value);

		/* process the variable/value */

		if(!strcmp(variable, "resource_file")) {

			/* save the macro */
			my_free(mac->x[MACRO_RESOURCEFILE]);
			mac->x[MACRO_RESOURCEFILE] = (char *)strdup(value);

			/* process the resource file */
			read_resource_file(value);
			}

		else if(!strcmp(variable, "log_file")) {

			if(strlen(value) > MAX_FILENAME_LENGTH - 1) {
				asprintf(&error_message, "Log file is too long");
				error = TRUE;
				break;
				}

			my_free(log_file);
			log_file = (char *)strdup(value);

			/* save the macro */
			my_free(mac->x[MACRO_LOGFILE]);
			mac->x[MACRO_LOGFILE] = (char *)strdup(log_file);
			}

		else if(!strcmp(variable, "debug_level"))
			debug_level = atoi(value);

		else if(!strcmp(variable, "debug_verbosity"))
			debug_verbosity = atoi(value);

		else if(!strcmp(variable, "debug_file")) {

			if(strlen(value) > MAX_FILENAME_LENGTH - 1) {
				asprintf(&error_message, "Debug log file is too long");
				error = TRUE;
				break;
				}

			my_free(debug_file);
			debug_file = (char *)strdup(value);
			}

		else if(!strcmp(variable, "max_debug_file_size"))
			max_debug_file_size = strtoul(value, NULL, 0);

		else if(!strcmp(variable, "command_file")) {

			if(strlen(value) > MAX_FILENAME_LENGTH - 1) {
				asprintf(&error_message, "Command file is too long");
				error = TRUE;
				break;
				}

			my_free(command_file);
			command_file = (char *)strdup(value);

			/* save the macro */
			my_free(mac->x[MACRO_COMMANDFILE]);
			mac->x[MACRO_COMMANDFILE] = (char *)strdup(value);
			}

		else if(!strcmp(variable, "temp_file")) {

			if(strlen(value) > MAX_FILENAME_LENGTH - 1) {
				asprintf(&error_message, "Temp file is too long");
				error = TRUE;
				break;
				}

			my_free(temp_file);
			temp_file = (char *)strdup(value);

			/* save the macro */
			my_free(mac->x[MACRO_TEMPFILE]);
			mac->x[MACRO_TEMPFILE] = (char *)strdup(temp_file);
			}

		else if(!strcmp(variable, "temp_path")) {

			if(strlen(value) > MAX_FILENAME_LENGTH - 1) {
				asprintf(&error_message, "Temp path is too long");
				error = TRUE;
				break;
				}

			if((tmpdir = opendir((char *)value)) == NULL) {
				asprintf(&error_message, "Temp path is not a valid directory");
				error = TRUE;
				break;
				}
			closedir(tmpdir);

			my_free(temp_path);
			if((temp_path = (char *)strdup(value))) {
				strip(temp_path);
				/* make sure we don't have a trailing slash */
				if(temp_path[strlen(temp_path) - 1] == '/')
					temp_path[strlen(temp_path) - 1] = '\x0';
				}

			/* save the macro */
			my_free(mac->x[MACRO_TEMPPATH]);
			mac->x[MACRO_TEMPPATH] = (char *)strdup(temp_path);
			}

		else if(!strcmp(variable, "check_result_path")) {

			if(strlen(value) > MAX_FILENAME_LENGTH - 1) {
				asprintf(&error_message, "Check result path is too long");
				error = TRUE;
				break;
				}

			if((tmpdir = opendir((char *)value)) == NULL) {
				asprintf(&error_message, "Check result path is not a valid directory");
				error = TRUE;
				break;
				}
			closedir(tmpdir);

			my_free(temp_path);
			if((temp_path = (char *)strdup(value))) {
				strip(temp_path);
				/* make sure we don't have a trailing slash */
				if(temp_path[strlen(temp_path) - 1] == '/')
					temp_path[strlen(temp_path) - 1] = '\x0';
				}

			my_free(check_result_path);
			check_result_path = (char *)strdup(temp_path);
			}

		else if(!strcmp(variable, "max_check_result_file_age"))
			max_check_result_file_age = strtoul(value, NULL, 0);

		else if(!strcmp(variable, "lock_file")) {

			if(strlen(value) > MAX_FILENAME_LENGTH - 1) {
				asprintf(&error_message, "Lock file is too long");
				error = TRUE;
				break;
				}

			my_free(lock_file);
			lock_file = (char *)strdup(value);
			}

		else if(!strcmp(variable, "global_host_event_handler")) {
			my_free(global_host_event_handler);
			global_host_event_handler = (char *)strdup(value);
			}

		else if(!strcmp(variable, "global_service_event_handler")) {
			my_free(global_service_event_handler);
			global_service_event_handler = (char *)strdup(value);
			}

		else if(!strcmp(variable, "ocsp_command")) {
			my_free(ocsp_command);
			ocsp_command = (char *)strdup(value);
			}

		else if(!strcmp(variable, "ochp_command")) {
			my_free(ochp_command);
			ochp_command = (char *)strdup(value);
			}

		else if(!strcmp(variable, "nagios_user")) {
			my_free(nagios_user);
			nagios_user = (char *)strdup(value);
			}

		else if(!strcmp(variable, "nagios_group")) {
			my_free(nagios_group);
			nagios_group = (char *)strdup(value);
			}

		else if(!strcmp(variable, "admin_email")) {

			/* save the macro */
			my_free(mac->x[MACRO_ADMINEMAIL]);
			mac->x[MACRO_ADMINEMAIL] = (char *)strdup(value);
			}

		else if(!strcmp(variable, "admin_pager")) {

			/* save the macro */
			my_free(mac->x[MACRO_ADMINPAGER]);
			mac->x[MACRO_ADMINPAGER] = (char *)strdup(value);
			}

		else if(!strcmp(variable, "use_syslog")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for use_syslog");
				error = TRUE;
				break;
				}

			use_syslog = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "log_notifications")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for log_notifications");
				error = TRUE;
				break;
				}

			log_notifications = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "log_service_retries")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for log_service_retries");
				error = TRUE;
				break;
				}

			log_service_retries = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "log_host_retries")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for log_host_retries");
				error = TRUE;
				break;
				}

			log_host_retries = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "log_event_handlers")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for log_event_handlers");
				error = TRUE;
				break;
				}

			log_event_handlers = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "log_external_commands")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for log_external_commands");
				error = TRUE;
				break;
				}

			log_external_commands = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "log_passive_checks")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for log_passive_checks");
				error = TRUE;
				break;
				}

			log_passive_checks = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "log_initial_states")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for log_initial_states");
				error = TRUE;
				break;
				}

			log_initial_states = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "retain_state_information")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for retain_state_information");
				error = TRUE;
				break;
				}

			retain_state_information = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "retention_update_interval")) {

			retention_update_interval = atoi(value);
			if(retention_update_interval < 0) {
				asprintf(&error_message, "Illegal value for retention_update_interval");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "use_retained_program_state")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for use_retained_program_state");
				error = TRUE;
				break;
				}

			use_retained_program_state = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "use_retained_scheduling_info")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for use_retained_scheduling_info");
				error = TRUE;
				break;
				}

			use_retained_scheduling_info = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "retention_scheduling_horizon")) {

			retention_scheduling_horizon = atoi(value);

			if(retention_scheduling_horizon <= 0) {
				asprintf(&error_message, "Illegal value for retention_scheduling_horizon");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "additional_freshness_latency"))
			additional_freshness_latency = atoi(value);

		else if(!strcmp(variable, "retained_host_attribute_mask"))
			retained_host_attribute_mask = strtoul(value, NULL, 0);

		else if(!strcmp(variable, "retained_service_attribute_mask"))
			retained_service_attribute_mask = strtoul(value, NULL, 0);

		else if(!strcmp(variable, "retained_process_host_attribute_mask"))
			retained_process_host_attribute_mask = strtoul(value, NULL, 0);

		else if(!strcmp(variable, "retained_process_service_attribute_mask"))
			retained_process_service_attribute_mask = strtoul(value, NULL, 0);

		else if(!strcmp(variable, "retained_contact_host_attribute_mask"))
			retained_contact_host_attribute_mask = strtoul(value, NULL, 0);

		else if(!strcmp(variable, "retained_contact_service_attribute_mask"))
			retained_contact_service_attribute_mask = strtoul(value, NULL, 0);

		else if(!strcmp(variable, "obsess_over_services")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for obsess_over_services");
				error = TRUE;
				break;
				}

			obsess_over_services = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "obsess_over_hosts")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for obsess_over_hosts");
				error = TRUE;
				break;
				}

			obsess_over_hosts = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "translate_passive_host_checks")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for translate_passive_host_checks");
				error = TRUE;
				break;
				}

			translate_passive_host_checks = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "passive_host_checks_are_soft"))
			passive_host_checks_are_soft = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "service_check_timeout")) {

			service_check_timeout = atoi(value);

			if(service_check_timeout <= 0) {
				asprintf(&error_message, "Illegal value for service_check_timeout");
				error = TRUE;
				break;
				}
			}
        
		else if(!strcmp(variable,"service_check_timeout_state")){

			if(!strcmp(value,"o"))
				service_check_timeout_state=STATE_OK;
			else if(!strcmp(value,"w"))
				service_check_timeout_state=STATE_WARNING;
			else if(!strcmp(value,"c"))
				service_check_timeout_state=STATE_CRITICAL;
			else if(!strcmp(value,"u"))
				service_check_timeout_state=STATE_UNKNOWN;
			else{
				asprintf(&error_message,"Illegal value for service_check_timeout_state");
				error=TRUE;
				break;
					}
				}

		else if(!strcmp(variable, "host_check_timeout")) {

			host_check_timeout = atoi(value);

			if(host_check_timeout <= 0) {
				asprintf(&error_message, "Illegal value for host_check_timeout");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "event_handler_timeout")) {

			event_handler_timeout = atoi(value);

			if(event_handler_timeout <= 0) {
				asprintf(&error_message, "Illegal value for event_handler_timeout");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "notification_timeout")) {

			notification_timeout = atoi(value);

			if(notification_timeout <= 0) {
				asprintf(&error_message, "Illegal value for notification_timeout");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "ocsp_timeout")) {

			ocsp_timeout = atoi(value);

			if(ocsp_timeout <= 0) {
				asprintf(&error_message, "Illegal value for ocsp_timeout");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "ochp_timeout")) {

			ochp_timeout = atoi(value);

			if(ochp_timeout <= 0) {
				asprintf(&error_message, "Illegal value for ochp_timeout");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "use_agressive_host_checking") || !strcmp(variable, "use_aggressive_host_checking")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for use_aggressive_host_checking");
				error = TRUE;
				break;
				}

			use_aggressive_host_checking = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "cached_host_check_horizon"))
			cached_host_check_horizon = strtoul(value, NULL, 0);

		else if(!strcmp(variable, "enable_predictive_host_dependency_checks"))
			enable_predictive_host_dependency_checks = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "cached_service_check_horizon"))
			cached_service_check_horizon = strtoul(value, NULL, 0);

		else if(!strcmp(variable, "enable_predictive_service_dependency_checks"))
			enable_predictive_service_dependency_checks = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "soft_state_dependencies")) {
			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for soft_state_dependencies");
				error = TRUE;
				break;
				}

			soft_state_dependencies = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "log_rotation_method")) {
			if(!strcmp(value, "n"))
				log_rotation_method = LOG_ROTATION_NONE;
			else if(!strcmp(value, "h"))
				log_rotation_method = LOG_ROTATION_HOURLY;
			else if(!strcmp(value, "d"))
				log_rotation_method = LOG_ROTATION_DAILY;
			else if(!strcmp(value, "w"))
				log_rotation_method = LOG_ROTATION_WEEKLY;
			else if(!strcmp(value, "m"))
				log_rotation_method = LOG_ROTATION_MONTHLY;
			else {
				asprintf(&error_message, "Illegal value for log_rotation_method");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "log_archive_path")) {

			if(strlen(value) > MAX_FILENAME_LENGTH - 1) {
				asprintf(&error_message, "Log archive path too long");
				error = TRUE;
				break;
				}

			my_free(log_archive_path);
			log_archive_path = (char *)strdup(value);
			}

		else if(!strcmp(variable, "enable_event_handlers"))
			enable_event_handlers = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "enable_notifications"))
			enable_notifications = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "execute_service_checks"))
			execute_service_checks = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "accept_passive_service_checks"))
			accept_passive_service_checks = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "execute_host_checks"))
			execute_host_checks = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "accept_passive_host_checks"))
			accept_passive_host_checks = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "service_inter_check_delay_method")) {
			if(!strcmp(value, "n"))
				service_inter_check_delay_method = ICD_NONE;
			else if(!strcmp(value, "d"))
				service_inter_check_delay_method = ICD_DUMB;
			else if(!strcmp(value, "s"))
				service_inter_check_delay_method = ICD_SMART;
			else {
				service_inter_check_delay_method = ICD_USER;
				scheduling_info.service_inter_check_delay = strtod(value, NULL);
				if(scheduling_info.service_inter_check_delay <= 0.0) {
					asprintf(&error_message, "Illegal value for service_inter_check_delay_method");
					error = TRUE;
					break;
					}
				}
			}

		else if(!strcmp(variable, "max_service_check_spread")) {
			strip(value);
			max_service_check_spread = atoi(value);
			if(max_service_check_spread < 1) {
				asprintf(&error_message, "Illegal value for max_service_check_spread");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "host_inter_check_delay_method")) {

			if(!strcmp(value, "n"))
				host_inter_check_delay_method = ICD_NONE;
			else if(!strcmp(value, "d"))
				host_inter_check_delay_method = ICD_DUMB;
			else if(!strcmp(value, "s"))
				host_inter_check_delay_method = ICD_SMART;
			else {
				host_inter_check_delay_method = ICD_USER;
				scheduling_info.host_inter_check_delay = strtod(value, NULL);
				if(scheduling_info.host_inter_check_delay <= 0.0) {
					asprintf(&error_message, "Illegal value for host_inter_check_delay_method");
					error = TRUE;
					break;
					}
				}
			}

		else if(!strcmp(variable, "max_host_check_spread")) {

			max_host_check_spread = atoi(value);
			if(max_host_check_spread < 1) {
				asprintf(&error_message, "Illegal value for max_host_check_spread");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "service_interleave_factor")) {
			if(!strcmp(value, "s"))
				service_interleave_factor_method = ILF_SMART;
			else {
				service_interleave_factor_method = ILF_USER;
				scheduling_info.service_interleave_factor = atoi(value);
				if(scheduling_info.service_interleave_factor < 1)
					scheduling_info.service_interleave_factor = 1;
				}
			}

		else if(!strcmp(variable, "max_concurrent_checks")) {

			max_parallel_service_checks = atoi(value);
			if(max_parallel_service_checks < 0) {
				asprintf(&error_message, "Illegal value for max_concurrent_checks");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "check_result_reaper_frequency") || !strcmp(variable, "service_reaper_frequency")) {

			check_reaper_interval = atoi(value);
			if(check_reaper_interval < 1) {
				asprintf(&error_message, "Illegal value for check_result_reaper_frequency");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "max_check_result_reaper_time")) {

			max_check_reaper_time = atoi(value);
			if(max_check_reaper_time < 1) {
				asprintf(&error_message, "Illegal value for max_check_result_reaper_time");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "sleep_time")) {
			obsoleted_warning(variable, NULL);
			}

		else if(!strcmp(variable, "interval_length")) {

			interval_length = atoi(value);
			if(interval_length < 1) {
				asprintf(&error_message, "Illegal value for interval_length");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "check_external_commands")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for check_external_commands");
				error = TRUE;
				break;
				}

			check_external_commands = (atoi(value) > 0) ? TRUE : FALSE;
			}

		/* @todo Remove before Nagios 4.3 */
		else if(!strcmp(variable, "command_check_interval")) {
			obsoleted_warning(variable, "Commands are always handled on arrival");
			}

		else if(!strcmp(variable, "check_for_orphaned_services")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for check_for_orphaned_services");
				error = TRUE;
				break;
				}

			check_orphaned_services = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "check_for_orphaned_hosts")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for check_for_orphaned_hosts");
				error = TRUE;
				break;
				}

			check_orphaned_hosts = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "check_service_freshness")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for check_service_freshness");
				error = TRUE;
				break;
				}

			check_service_freshness = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "check_host_freshness")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for check_host_freshness");
				error = TRUE;
				break;
				}

			check_host_freshness = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "service_freshness_check_interval") || !strcmp(variable, "freshness_check_interval")) {

			service_freshness_check_interval = atoi(value);
			if(service_freshness_check_interval <= 0) {
				asprintf(&error_message, "Illegal value for service_freshness_check_interval");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "host_freshness_check_interval")) {

			host_freshness_check_interval = atoi(value);
			if(host_freshness_check_interval <= 0) {
				asprintf(&error_message, "Illegal value for host_freshness_check_interval");
				error = TRUE;
				break;
				}
			}
		else if(!strcmp(variable, "auto_reschedule_checks")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for auto_reschedule_checks");
				error = TRUE;
				break;
				}

			auto_reschedule_checks = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "auto_rescheduling_interval")) {

			auto_rescheduling_interval = atoi(value);
			if(auto_rescheduling_interval <= 0) {
				asprintf(&error_message, "Illegal value for auto_rescheduling_interval");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "auto_rescheduling_window")) {

			auto_rescheduling_window = atoi(value);
			if(auto_rescheduling_window <= 0) {
				asprintf(&error_message, "Illegal value for auto_rescheduling_window");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "aggregate_status_updates")) {

			/* DEPRECATED - ALL UPDATES ARE AGGREGATED AS OF NAGIOS 3.X */
			/*aggregate_status_updates=(atoi(value)>0)?TRUE:FALSE;*/

			logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: aggregate_status_updates directive ignored.  All status file updates are now aggregated.");
			}

		else if(!strcmp(variable, "status_update_interval")) {

			status_update_interval = atoi(value);
			if(status_update_interval <= 1) {
				asprintf(&error_message, "Illegal value for status_update_interval");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "time_change_threshold")) {

			time_change_threshold = atoi(value);

			if(time_change_threshold <= 5) {
				asprintf(&error_message, "Illegal value for time_change_threshold");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "process_performance_data"))
			process_performance_data = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "enable_flap_detection"))
			enable_flap_detection = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "enable_failure_prediction"))
			enable_failure_prediction = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "low_service_flap_threshold")) {

			low_service_flap_threshold = strtod(value, NULL);
			if(low_service_flap_threshold <= 0.0 || low_service_flap_threshold >= 100.0) {
				asprintf(&error_message, "Illegal value for low_service_flap_threshold");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "high_service_flap_threshold")) {

			high_service_flap_threshold = strtod(value, NULL);
			if(high_service_flap_threshold <= 0.0 ||  high_service_flap_threshold > 100.0) {
				asprintf(&error_message, "Illegal value for high_service_flap_threshold");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "low_host_flap_threshold")) {

			low_host_flap_threshold = strtod(value, NULL);
			if(low_host_flap_threshold <= 0.0 || low_host_flap_threshold >= 100.0) {
				asprintf(&error_message, "Illegal value for low_host_flap_threshold");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "high_host_flap_threshold")) {

			high_host_flap_threshold = strtod(value, NULL);
			if(high_host_flap_threshold <= 0.0 || high_host_flap_threshold > 100.0) {
				asprintf(&error_message, "Illegal value for high_host_flap_threshold");
				error = TRUE;
				break;
				}
			}

		else if(!strcmp(variable, "date_format")) {

			if(!strcmp(value, "euro"))
				date_format = DATE_FORMAT_EURO;
			else if(!strcmp(value, "iso8601"))
				date_format = DATE_FORMAT_ISO8601;
			else if(!strcmp(value, "strict-iso8601"))
				date_format = DATE_FORMAT_STRICT_ISO8601;
			else
				date_format = DATE_FORMAT_US;
			}

		else if(!strcmp(variable, "use_timezone")) {
			my_free(use_timezone);
			use_timezone = (char *)strdup(value);
			}

		else if(!strcmp(variable, "event_broker_options")) {

			if(!strcmp(value, "-1"))
				event_broker_options = BROKER_EVERYTHING;
			else
				event_broker_options = strtoul(value, NULL, 0);
			}

		else if(!strcmp(variable, "illegal_object_name_chars"))
			illegal_object_chars = (char *)strdup(value);

		else if(!strcmp(variable, "illegal_macro_output_chars"))
			illegal_output_chars = (char *)strdup(value);


		else if(!strcmp(variable, "broker_module")) {
			/* WAS:	modptr = strtok(value, " \n");
				argptr = strtok(NULL, "\n"); */
			if ((argptr = strstr(value, " \n"))!=NULL) {
			      *argptr='\0';
			      argptr++;
			      argptr++;
			      if ((temp_ptr = strchr(argptr,'\n'))!=NULL)
				    *temp_ptr='\0';
#ifdef USE_EVENT_BROKER
			      /* input mod (value) and arg (argptr) are
			       * strduped in neb_add_module already */
			      neb_add_module(value, argptr, TRUE);
#endif
			      if (temp_ptr!=NULL) *temp_ptr='\n';
			      argptr--;
			      argptr--;
			      *argptr=' ';
			      }
			}

		else if(!strcmp(variable, "use_regexp_matching"))
			use_regexp_matches = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "use_true_regexp_matching"))
			use_true_regexp_matching = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "daemon_dumps_core")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for daemon_dumps_core");
				error = TRUE;
				break;
				}

			daemon_dumps_core = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "use_large_installation_tweaks")) {

			if(strlen(value) != 1 || value[0] < '0' || value[0] > '1') {
				asprintf(&error_message, "Illegal value for use_large_installation_tweaks ");
				error = TRUE;
				break;
				}

			use_large_installation_tweaks = (atoi(value) > 0) ? TRUE : FALSE;
			}

		else if(!strcmp(variable, "enable_environment_macros"))
			enable_environment_macros = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "free_child_process_memory"))
			free_child_process_memory = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "child_processes_fork_twice"))
			child_processes_fork_twice = (atoi(value) > 0) ? TRUE : FALSE;

		/*** embedded perl variables are deprecated now ***/
		else if(!strcmp(variable, "enable_embedded_perl"))
			obsoleted_warning(variable, NULL);
		else if(!strcmp(variable, "use_embedded_perl_implicitly"))
			obsoleted_warning(variable, NULL);
		else if(!strcmp(variable, "auth_file"))
			obsoleted_warning(variable, NULL);
		else if(!strcmp(variable, "p1_file"))
			obsoleted_warning(variable, NULL);

		/*** as is external_command_buffer_slots */
		else if(!strcmp(variable, "external_command_buffer_slots"))
			obsoleted_warning(variable, "All commands are always processed upon arrival");

		else if(!strcmp(variable, "check_for_updates"))
			check_for_updates = (atoi(value) > 0) ? TRUE : FALSE;

		else if(!strcmp(variable, "bare_update_check"))
			bare_update_check = (atoi(value) > 0) ? TRUE : FALSE;

		/* warn about old variables */
		else if(!strcmp(variable, "comment_file") || !strcmp(variable, "xcddefault_comment_file")) {
			logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: comment_file variable ignored.  Comments are now stored in the status and retention files.");
			}
		else if(!strcmp(variable, "downtime_file") || !strcmp(variable, "xdddefault_downtime_file")) {
			logit(NSLOG_CONFIG_WARNING, TRUE, "Warning: downtime_file variable ignored.  Downtime entries are now stored in the status and retention files.");
			}

		/* skip external data directives */
		else if(strstr(input, "x") == input)
			continue;

		/* ignore external variables */
		else if(!strcmp(variable, "status_file"))
			continue;
		else if(!strcmp(variable, "perfdata_timeout"))
			continue;
		else if(strstr(variable, "host_perfdata") == variable)
			continue;
		else if(strstr(variable, "service_perfdata") == variable)
			continue;
		else if(strstr(variable, "host_saveddata") == variable)
			continue;
		else if(strstr(variable, "service_saveddata") == variable)
			continue;
		else if(strstr(input, "cfg_file=") == input || strstr(input, "cfg_dir=") == input)
			continue;
		else if(strstr(input, "state_retention_file=") == input)
			continue;
		else if(strstr(input, "object_cache_file=") == input)
			continue;
		else if(strstr(input, "precached_object_file=") == input)
			continue;
		else if(!strcmp(variable, "allow_empty_hostgroup_assignment")) {
			allow_empty_hostgroup_assignment = (atoi(value) > 0) ? TRUE : FALSE;
			}

		/* we don't know what this variable is... */
		else {
			asprintf(&error_message, "UNKNOWN VARIABLE");
			error = TRUE;
			break;
			}

		}

	/* adjust timezone values */
	if(use_timezone != NULL)
		set_environment_var("TZ", use_timezone, 1);
	tzset();

	/* adjust tweaks */
	if(free_child_process_memory == -1)
		free_child_process_memory = (use_large_installation_tweaks == TRUE) ? FALSE : TRUE;
	if(child_processes_fork_twice == -1)
		child_processes_fork_twice = (use_large_installation_tweaks == TRUE) ? FALSE : TRUE;

	/* handle errors */
	if(error == TRUE) {
		logit(NSLOG_CONFIG_ERROR, TRUE, "Error in configuration file '%s' - Line %d (%s)", main_config_file, current_line, (error_message == NULL) ? "NULL" : error_message);
		return ERROR;
		}

	/* free leftover memory and close the file */
	my_free(input);
	mmap_fclose(thefile);

	/* free memory */
	my_free(error_message);
	my_free(input);
	my_free(variable);
	my_free(value);

	/* make sure a log file has been specified */
	strip(log_file);
	if(!strcmp(log_file, "")) {
		if(daemon_mode == FALSE)
			printf("Error: Log file is not specified anywhere in main config file '%s'!\n", main_config_file);
		return ERROR;
		}

	return OK;
	}

/* processes macros in resource file */
int read_resource_file(char *resource_file) {
	char *input = NULL;
	char *variable = NULL;
	char *value = NULL;
	mmapfile *thefile = NULL;
	int current_line = 1;
	int error = FALSE;
	int user_index = 0;

	if((thefile = mmap_fopen(resource_file)) == NULL) {
		logit(NSLOG_CONFIG_ERROR, TRUE, "Error: Cannot open resource file '%s' for reading!", resource_file);
		return ERROR;
		}

	/* process all lines in the resource file */
	while(1) {

		/* free memory */
		my_free(input);
		my_free(variable);
		my_free(value);

		/* read the next line */
		if((input = mmap_fgets_multiline(thefile)) == NULL)
			break;

		current_line = thefile->current_line;

		/* skip blank lines and comments */
		if(input[0] == '#' || input[0] == '\x0' || input[0] == '\n' || input[0] == '\r')
			continue;

		strip(input);

		/* get the variable and value pair */
		if (!my_str2parts(input,'=', &variable,&value)) {
			logit(NSLOG_CONFIG_ERROR, TRUE, "Error: Bad variable declaration - Line %d of resource file '%s'", current_line, resource_file);
			error = TRUE;
			break;
			}

		/* what should we do with the variable/value pair? */

		/* check for macro declarations */
		if(variable[0] == '$' && variable[strlen(variable) - 1] == '$') {

			/* $USERx$ macro declarations */
			if(strstr(variable, "$USER") == variable  && strlen(variable) > 5) {
				user_index = atoi(variable + 5) - 1;
				if(user_index >= 0 && user_index < MAX_USER_MACROS) {
					my_free(macro_user[user_index]);
					macro_user[user_index] = (char *)strdup(value);
					}
				}
			}
		}

	/* free leftover memory and close the file */
	my_free(input);
	mmap_fclose(thefile);

	/* free memory */
	my_free(variable);
	my_free(value);

	if(error == TRUE)
		return ERROR;

	return OK;
	}

	
/****************************************************************/
/**************** CONFIG VERIFICATION FUNCTIONS *****************/
/****************************************************************/

/* do a pre-flight check to make sure object relationships, etc. make sense */
int pre_flight_check(void) {
	char *buf = NULL;
	int warnings = 0;
	int errors = 0;
	struct timeval tv[4];
	double runtime[4];
	int temp_path_fd = -1;


        if(test_scheduling == TRUE)
                gettimeofday(&tv[0], NULL);

        /********************************************/
        /* check object relationships               */
        /********************************************/
	pre_flight_object_check(&warnings, &errors);
	if(test_scheduling == TRUE)
		gettimeofday(&tv[1], NULL);

	/********************************************/
	/* check for circular paths between hosts   */
	/********************************************/
	pre_flight_circular_check(&warnings, &errors);
	if(test_scheduling == TRUE)
		gettimeofday(&tv[2], NULL);


	/********************************************/
	/* check global event handler commands...   */
	/********************************************/
	if(verify_config == TRUE)
		printf("Checking global event handlers...\n");

	if(global_host_event_handler != NULL) {
		global_host_event_handler_ptr = find_bang_command(global_host_event_handler);
		if (global_host_event_handler_ptr == NULL) {
			logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Global host event handler command '%s' is not defined anywhere!", global_host_event_handler);
			errors++;
			}
		}

	if(global_service_event_handler != NULL) {
		global_service_event_handler_ptr = find_bang_command(global_service_event_handler);
		if (global_service_event_handler_ptr == NULL) {
			logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Global service event handler command '%s' is not defined anywhere!", global_service_event_handler);
			errors++;
			}
		}

	/**************************************************/
	/* check obsessive processor commands...          */
	/**************************************************/
	if(verify_config == TRUE)
		printf("Checking obsessive compulsive processor commands...\n");

	if(ocsp_command != NULL) {
		ocsp_command_ptr = find_bang_command(ocsp_command);
		if (!ocsp_command_ptr) {
			logit(NSLOG_CONFIG_ERROR, TRUE, "Error: OCSP command '%s' is not defined anywhere!\n", ocsp_command);
			errors++;
			}
		}
	if(ochp_command != NULL) {
		ochp_command_ptr = find_bang_command(ochp_command);
		logit(NSLOG_CONFIG_ERROR, TRUE, "Error: OCHP command '%s' is not defined anywhere!\n", ochp_command);
		ochp_command_ptr = find_bang_command(ochp_command);
		errors += ochp_command_ptr == NULL;
		}

	/**************************************************/
	/* check various settings...                      */
	/**************************************************/
	if(verify_config == TRUE)
		printf("Checking misc settings...\n");

	/* check if we can write to temp_path */
	if (asprintf(&buf, "%s/nagiosXXXXXX", temp_path) == -1 ||
	    (temp_path_fd = mkstemp(buf)) == -1) {
		logit(NSLOG_VERIFICATION_ERROR, TRUE, "\tError: Unable to write to temp_path ('%s') - %s\n", temp_path, strerror(errno));
		errors++;
		}
	else {
		close(temp_path_fd);
		remove(buf);
		}
	my_free(buf);

	/* check if we can write to check_result_path */
	if (asprintf(&buf, "%s/nagiosXXXXXX", check_result_path) == -1 ||
	    (temp_path_fd = mkstemp(buf)) == -1) {
		logit(NSLOG_VERIFICATION_WARNING, TRUE, "\tError: Unable to write to check_result_path ('%s') - %s\n", check_result_path, strerror(errno));
		errors++;
		}
	else {
		close(temp_path_fd);
		remove(buf);
		}
	my_free(buf);

	/* warn if user didn't specify any illegal macro output chars */
	if(illegal_output_chars == NULL) {
		logit(NSLOG_VERIFICATION_WARNING, TRUE, "%s", "Warning: Nothing specified for illegal_macro_output_chars variable!\n");
		warnings++;
		}

	if(verify_config == TRUE) {
		printf("\n");
		printf("Total Warnings: %d\n", warnings);
		printf("Total Errors:   %d\n", errors);
		}

	if(test_scheduling == TRUE)
		gettimeofday(&tv[3], NULL);

	if(test_scheduling == TRUE) {

		if(verify_object_relationships == TRUE)
			runtime[0] = (double)((double)(tv[1].tv_sec - tv[0].tv_sec) + (double)((tv[1].tv_usec - tv[0].tv_usec) / 1000.0) / 1000.0);
		else
			runtime[0] = 0.0;
		if(verify_circular_paths == TRUE)
			runtime[1] = (double)((double)(tv[2].tv_sec - tv[1].tv_sec) + (double)((tv[2].tv_usec - tv[1].tv_usec) / 1000.0) / 1000.0);
		else
			runtime[1] = 0.0;
		runtime[2] = (double)((double)(tv[3].tv_sec - tv[2].tv_sec) + (double)((tv[3].tv_usec - tv[2].tv_usec) / 1000.0) / 1000.0);
		runtime[3] = runtime[0] + runtime[1] + runtime[2];

		printf("Timing information on configuration verification is listed below.\n\n");

		printf("CONFIG VERIFICATION TIMES          (* = Potential for speedup with -x option)\n");
		printf("----------------------------------\n");
		printf("Object Relationships: %.6lf sec\n", runtime[0]);
		printf("Circular Paths:       %.6lf sec  *\n", runtime[1]);
		printf("Misc:                 %.6lf sec\n", runtime[2]);
		printf("                      ============\n");
		printf("TOTAL:                %.6lf sec  * = %.6lf sec (%.1f%%) estimated savings\n", runtime[3], runtime[1], (runtime[1] / runtime[3]) * 100.0);
		printf("\n\n");
		}

	return (errors > 0) ? ERROR : OK;
	}

/* do a pre-flight check to make sure object relationships make sense */
int pre_flight_object_check(int *w, int *e) {
	contact *temp_contact = NULL;
	commandsmember *temp_commandsmember = NULL;
	contactgroup *temp_contactgroup = NULL;
	contactsmember *temp_contactsmember = NULL;
	contactgroupsmember *temp_contactgroupsmember = NULL;
	host *temp_host = NULL;
	host *temp_host2 = NULL;
	hostsmember *temp_hostsmember = NULL;
	hostgroup *temp_hostgroup = NULL;
	servicegroup *temp_servicegroup = NULL;
	servicesmember *temp_servicesmember = NULL;
	service *temp_service = NULL;
	command *temp_command = NULL;
	timeperiod *temp_timeperiod = NULL;
	timeperiod *temp_timeperiod2 = NULL;
	timeperiodexclusion *temp_timeperiodexclusion = NULL;
	int total_objects = 0;
	int warnings = 0;
	int errors = 0;

#ifdef TEST
	void *ptr = NULL;
	char *buf1 = "";
	char *buf2 = "";
	buf1 = "temptraxe1";
	buf2 = "Probe 2";
	for(temp_se = get_first_serviceescalation_by_service(buf1, buf2, &ptr); temp_se != NULL; temp_se = get_next_serviceescalation_by_service(buf1, buf2, &ptr)) {
		printf("FOUND ESCALATION FOR SVC '%s'/'%s': %d-%d/%.3f, PTR=%p\n", buf1, buf2, temp_se->first_notification, temp_se->last_notification, temp_se->notification_interval, ptr);
		}
	for(temp_he = get_first_hostescalation_by_host(buf1, &ptr); temp_he != NULL; temp_he = get_next_hostescalation_by_host(buf1, &ptr)) {
		printf("FOUND ESCALATION FOR HOST '%s': %d-%d/%d, PTR=%p\n", buf1, temp_he->first_notification, temp_he->last_notification, temp_he->notification_interval, ptr);
		}
#endif

	/* bail out if we aren't supposed to verify object relationships */
	if(verify_object_relationships == FALSE)
		return OK;

	/*****************************************/
	/* check each service...                 */
	/*****************************************/
	if(verify_config == TRUE)
		printf("Checking services...\n");
	if(get_service_count() == 0) {
		logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: There are no services defined!");
		errors++;
		}
	total_objects = 0;
	for(temp_service = service_list; temp_service != NULL; temp_service = temp_service->next) {

		total_objects++;

		/* check the event handler command */
		if(temp_service->event_handler != NULL) {
			temp_service->event_handler_ptr = find_bang_command(temp_service->event_handler);
			if(temp_service->event_handler_ptr == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Event handler command '%s' specified in service '%s' for host '%s' not defined anywhere", temp_service->event_handler, temp_service->description, temp_service->host_name);
				errors++;
				}
			}

		/* check the service check_command */
		temp_service->check_command_ptr = find_bang_command(temp_service->service_check_command);
		if(temp_service->check_command_ptr == NULL) {
			logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Service check command '%s' specified in service '%s' for host '%s' not defined anywhere!", temp_service->service_check_command, temp_service->description, temp_service->host_name);
			errors++;
			}

		/* check for sane recovery options */
		if(temp_service->notify_on_recovery == TRUE && temp_service->notify_on_warning == FALSE && temp_service->notify_on_critical == FALSE) {
			logit(NSLOG_VERIFICATION_WARNING, TRUE, "Warning: Recovery notification option in service '%s' for host '%s' doesn't make any sense - specify warning and/or critical options as well", temp_service->description, temp_service->host_name);
			warnings++;
			}

		/* check to see if there is at least one contact/group */
		if(temp_service->contacts == NULL && temp_service->contact_groups == NULL) {
			logit(NSLOG_VERIFICATION_WARNING, TRUE, "Warning: Service '%s' on host '%s' has no default contacts or contactgroups defined!", temp_service->description, temp_service->host_name);
			warnings++;
			}

		/* verify service check timeperiod */
		if(temp_service->check_period == NULL) {
			logit(NSLOG_VERIFICATION_WARNING, TRUE, "Warning: Service '%s' on host '%s' has no check time period defined!", temp_service->description, temp_service->host_name);
			warnings++;
			}

		/* check service notification timeperiod */
		if(temp_service->notification_period == NULL) {
			logit(NSLOG_VERIFICATION_WARNING, TRUE, "Warning: Service '%s' on host '%s' has no notification time period defined!", temp_service->description, temp_service->host_name);
			warnings++;
			}

		/* see if the notification interval is less than the check interval */
		if(temp_service->notification_interval < temp_service->check_interval && temp_service->notification_interval != 0) {
			logit(NSLOG_VERIFICATION_WARNING, TRUE, "Warning: Service '%s' on host '%s'  has a notification interval less than its check interval!  Notifications are only re-sent after checks are made, so the effective notification interval will be that of the check interval.", temp_service->description, temp_service->host_name);
			warnings++;
			}

		/* check for illegal characters in service description */
		if(use_precached_objects == FALSE) {
			if(contains_illegal_object_chars(temp_service->description) == TRUE) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: The description string for service '%s' on host '%s' contains one or more illegal characters.", temp_service->description, temp_service->host_name);
				errors++;
				}
			}
		}

	if(verify_config == TRUE)
		printf("\tChecked %d services.\n", total_objects);

	/*****************************************/
	/* check all hosts...                    */
	/*****************************************/
	if(verify_config == TRUE)
		printf("Checking hosts...\n");

	if(get_host_count() == 0) {
		logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: There are no hosts defined!");
		errors++;
		}

	total_objects = 0;
	for(temp_host = host_list; temp_host != NULL; temp_host = temp_host->next) {

		total_objects++;

		/* make sure each host has at least one service associated with it */
		if(temp_host->total_services == 0) {
			logit(NSLOG_VERIFICATION_WARNING, TRUE, "Warning: Host '%s' has no services associated with it!", temp_host->name);
			warnings++;
			}

		/* check the event handler command */
		if(temp_host->event_handler != NULL) {
			temp_host->event_handler_ptr = find_bang_command(temp_host->event_handler);
			if(temp_host->event_handler_ptr == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Event handler command '%s' specified for host '%s' not defined anywhere", temp_host->event_handler, temp_host->name);
				errors++;
				}
			}

		/* hosts that don't have check commands defined shouldn't ever be checked... */
		if(temp_host->host_check_command != NULL) {
			temp_host->check_command_ptr = find_bang_command(temp_host->host_check_command);
			if(temp_host->check_command_ptr == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Host check command '%s' specified for host '%s' is not defined anywhere!", temp_host->host_check_command, temp_host->name);
				errors++;
				}
			}

		/* check host check timeperiod */
		if(temp_host->check_period != NULL) {
			temp_timeperiod = find_timeperiod(temp_host->check_period);
			if(temp_timeperiod == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Check period '%s' specified for host '%s' is not defined anywhere!", temp_host->check_period, temp_host->name);
				errors++;
				}

			/* save the pointer to the check timeperiod for later */
			temp_host->check_period_ptr = temp_timeperiod;
			}

		/* check all contacts */
		for(temp_contactsmember = temp_host->contacts; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {

			temp_contact = find_contact(temp_contactsmember->contact_name);

			if(temp_contact == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Contact '%s' specified in host '%s' is not defined anywhere!", temp_contactsmember->contact_name, temp_host->name);
				errors++;
				}

			/* save the contact pointer for later */
			temp_contactsmember->contact_ptr = temp_contact;
			}

		/* check all contact groups */
		for(temp_contactgroupsmember = temp_host->contact_groups; temp_contactgroupsmember != NULL; temp_contactgroupsmember = temp_contactgroupsmember->next) {

			temp_contactgroup = find_contactgroup(temp_contactgroupsmember->group_name);

			if(temp_contactgroup == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Contact group '%s' specified in host '%s' is not defined anywhere!", temp_contactgroupsmember->group_name, temp_host->name);
				errors++;
				}

			/* save the contact group pointer for later */
			temp_contactgroupsmember->group_ptr = temp_contactgroup;
			}

		/* check to see if there is at least one contact/group */
		if(temp_host->contacts == NULL && temp_host->contact_groups == NULL) {
			logit(NSLOG_VERIFICATION_WARNING, TRUE, "Warning: Host '%s' has no default contacts or contactgroups defined!", temp_host->name);
			warnings++;
			}

		/* check notification timeperiod */
		if(temp_host->notification_period != NULL) {
			temp_timeperiod = find_timeperiod(temp_host->notification_period);
			if(temp_timeperiod == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Notification period '%s' specified for host '%s' is not defined anywhere!", temp_host->notification_period, temp_host->name);
				errors++;
				}

			/* save the pointer to the notification timeperiod for later */
			temp_host->notification_period_ptr = temp_timeperiod;
			}

		/* check all parent parent host */
		for(temp_hostsmember = temp_host->parent_hosts; temp_hostsmember != NULL; temp_hostsmember = temp_hostsmember->next) {

			if((temp_host2 = find_host(temp_hostsmember->host_name)) == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: '%s' is not a valid parent for host '%s'!", temp_hostsmember->host_name, temp_host->name);
				errors++;
				}

			/* save the parent host pointer for later */
			temp_hostsmember->host_ptr = temp_host2;

			/* add a reverse (child) link to make searches faster later on */
			add_child_link_to_host(temp_host2, temp_host);
			}

		/* check for sane recovery options */
		if(temp_host->notify_on_recovery == TRUE && temp_host->notify_on_down == FALSE && temp_host->notify_on_unreachable == FALSE) {
			logit(NSLOG_VERIFICATION_WARNING, TRUE, "Warning: Recovery notification option in host '%s' definition doesn't make any sense - specify down and/or unreachable options as well", temp_host->name);
			warnings++;
			}

		/* check for illegal characters in host name */
		if(use_precached_objects == FALSE) {
			if(contains_illegal_object_chars(temp_host->name) == TRUE) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: The name of host '%s' contains one or more illegal characters.", temp_host->name);
				errors++;
				}
			}
		}

	if(verify_config == TRUE)
		printf("\tChecked %d hosts.\n", total_objects);

	/*****************************************/
	/* check each host group...              */
	/*****************************************/
	if(verify_config == TRUE)
		printf("Checking host groups...\n");
	for(temp_hostgroup = hostgroup_list, total_objects = 0; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next, total_objects++) {

		/* check all group members */
		for(temp_hostsmember = temp_hostgroup->members; temp_hostsmember != NULL; temp_hostsmember = temp_hostsmember->next) {

			temp_host = find_host(temp_hostsmember->host_name);
			if(temp_host == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Host '%s' specified in host group '%s' is not defined anywhere!", temp_hostsmember->host_name, temp_hostgroup->group_name);
				errors++;
				}

			/* save a pointer to this hostgroup for faster host/group membership lookups later */
			else
				add_object_to_objectlist(&temp_host->hostgroups_ptr, (void *)temp_hostgroup);

			/* save host pointer for later */
			temp_hostsmember->host_ptr = temp_host;
			}

		/* check for illegal characters in hostgroup name */
		if(use_precached_objects == FALSE) {
			if(contains_illegal_object_chars(temp_hostgroup->group_name) == TRUE) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: The name of hostgroup '%s' contains one or more illegal characters.", temp_hostgroup->group_name);
				errors++;
				}
			}
		}

	if(verify_config == TRUE)
		printf("\tChecked %d host groups.\n", total_objects);

	/*****************************************/
	/* check each service group...           */
	/*****************************************/
	if(verify_config == TRUE)
		printf("Checking service groups...\n");
	for(temp_servicegroup = servicegroup_list, total_objects = 0; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next, total_objects++) {

		/* check all group members */
		for(temp_servicesmember = temp_servicegroup->members; temp_servicesmember != NULL; temp_servicesmember = temp_servicesmember->next) {

			temp_service = find_service(temp_servicesmember->host_name, temp_servicesmember->service_description);
			if(temp_service == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Service '%s' on host '%s' specified in service group '%s' is not defined anywhere!", temp_servicesmember->service_description, temp_servicesmember->host_name, temp_servicegroup->group_name);
				errors++;
				}

			/* save a pointer to this servicegroup for faster service/group membership lookups later */
			else
				add_object_to_objectlist(&temp_service->servicegroups_ptr, (void *)temp_servicegroup);

			/* save service pointer for later */
			temp_servicesmember->service_ptr = temp_service;
			}

		/* check for illegal characters in servicegroup name */
		if(use_precached_objects == FALSE) {
			if(contains_illegal_object_chars(temp_servicegroup->group_name) == TRUE) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: The name of servicegroup '%s' contains one or more illegal characters.", temp_servicegroup->group_name);
				errors++;
				}
			}
		}

	if(verify_config == TRUE)
		printf("\tChecked %d service groups.\n", total_objects);

	/*****************************************/
	/* check all contacts...                 */
	/*****************************************/
	if(verify_config == TRUE)
		printf("Checking contacts...\n");
	if(contact_list == NULL) {
		logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: There are no contacts defined!");
		errors++;
		}
	for(temp_contact = contact_list, total_objects = 0; temp_contact != NULL; temp_contact = temp_contact->next, total_objects++) {

		/* check service notification commands */
		if(temp_contact->service_notification_commands == NULL) {
			logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Contact '%s' has no service notification commands defined!", temp_contact->name);
			errors++;
			}
		else for(temp_commandsmember = temp_contact->service_notification_commands; temp_commandsmember != NULL; temp_commandsmember = temp_commandsmember->next) {
			temp_commandsmember->command_ptr = find_bang_command(temp_commandsmember->command);
			if(temp_commandsmember->command_ptr == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Service notification command '%s' specified for contact '%s' is not defined anywhere!", temp_commandsmember->command, temp_contact->name);
				errors++;
				}
			}

		/* check host notification commands */
		if(temp_contact->host_notification_commands == NULL) {
			logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Contact '%s' has no host notification commands defined!", temp_contact->name);
			errors++;
			}
		else for(temp_commandsmember = temp_contact->host_notification_commands; temp_commandsmember != NULL; temp_commandsmember = temp_commandsmember->next) {
			temp_commandsmember->command_ptr = find_bang_command(temp_commandsmember->command);
			if(temp_commandsmember->command_ptr == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Host notification command '%s' specified for contact '%s' is not defined anywhere!", temp_commandsmember->command, temp_contact->name);
				errors++;
				}
			}

		/* check service notification timeperiod */
		if(temp_contact->service_notification_period == NULL) {
			logit(NSLOG_VERIFICATION_WARNING, TRUE, "Warning: Contact '%s' has no service notification time period defined!", temp_contact->name);
			warnings++;
			}

		else {
			temp_timeperiod = find_timeperiod(temp_contact->service_notification_period);
			if(temp_timeperiod == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Service notification period '%s' specified for contact '%s' is not defined anywhere!", temp_contact->service_notification_period, temp_contact->name);
				errors++;
				}

			/* save the pointer to the service notification timeperiod for later */
			temp_contact->service_notification_period_ptr = temp_timeperiod;
			}

		/* check host notification timeperiod */
		if(temp_contact->host_notification_period == NULL) {
			logit(NSLOG_VERIFICATION_WARNING, TRUE, "Warning: Contact '%s' has no host notification time period defined!", temp_contact->name);
			warnings++;
			}

		else {
			temp_timeperiod = find_timeperiod(temp_contact->host_notification_period);
			if(temp_timeperiod == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Host notification period '%s' specified for contact '%s' is not defined anywhere!", temp_contact->host_notification_period, temp_contact->name);
				errors++;
				}

			/* save the pointer to the host notification timeperiod for later */
			temp_contact->host_notification_period_ptr = temp_timeperiod;
			}

		/* check for sane host recovery options */
		if(temp_contact->notify_on_host_recovery == TRUE && temp_contact->notify_on_host_down == FALSE && temp_contact->notify_on_host_unreachable == FALSE) {
			logit(NSLOG_VERIFICATION_WARNING, TRUE, "Warning: Host recovery notification option for contact '%s' doesn't make any sense - specify down and/or unreachable options as well", temp_contact->name);
			warnings++;
			}

		/* check for sane service recovery options */
		if(temp_contact->notify_on_service_recovery == TRUE && temp_contact->notify_on_service_critical == FALSE && temp_contact->notify_on_service_warning == FALSE) {
			logit(NSLOG_VERIFICATION_WARNING, TRUE, "Warning: Service recovery notification option for contact '%s' doesn't make any sense - specify critical and/or warning options as well", temp_contact->name);
			warnings++;
			}

		/* check for illegal characters in contact name */
		if(use_precached_objects == FALSE) {
			if(contains_illegal_object_chars(temp_contact->name) == TRUE) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: The name of contact '%s' contains one or more illegal characters.", temp_contact->name);
				errors++;
				}
			}
		}

	if(verify_config == TRUE)
		printf("\tChecked %d contacts.\n", total_objects);

	/*****************************************/
	/* check each contact group...           */
	/*****************************************/
	if(verify_config == TRUE)
		printf("Checking contact groups...\n");
	for(temp_contactgroup = contactgroup_list, total_objects = 0; temp_contactgroup != NULL; temp_contactgroup = temp_contactgroup->next, total_objects++) {

		/* check all the group members */
		for(temp_contactsmember = temp_contactgroup->members; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {

			temp_contact = find_contact(temp_contactsmember->contact_name);
			if(temp_contact == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Contact '%s' specified in contact group '%s' is not defined anywhere!", temp_contactsmember->contact_name, temp_contactgroup->group_name);
				errors++;
				}

			/* save a pointer to this contactgroup for faster contact/group membership lookups later */
			else
				add_object_to_objectlist(&temp_contact->contactgroups_ptr, (void *)temp_contactgroup);

			/* save the contact pointer for later */
			temp_contactsmember->contact_ptr = temp_contact;
			}

		/* check for illegal characters in contactgroup name */
		if(use_precached_objects == FALSE) {
			if(contains_illegal_object_chars(temp_contactgroup->group_name) == TRUE) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: The name of contact group '%s' contains one or more illegal characters.", temp_contactgroup->group_name);
				errors++;
				}
			}
		}

	if(verify_config == TRUE)
		printf("\tChecked %d contact groups.\n", total_objects);


	/*****************************************/
	/* check all commands...                 */
	/*****************************************/
	if(verify_config == TRUE)
		printf("Checking commands...\n");

	for(temp_command = command_list, total_objects = 0; temp_command != NULL; temp_command = temp_command->next, total_objects++) {

		/* check for illegal characters in command name */
		if(use_precached_objects == FALSE) {
			if(contains_illegal_object_chars(temp_command->name) == TRUE) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: The name of command '%s' contains one or more illegal characters.", temp_command->name);
				errors++;
				}
			}
		}

	if(verify_config == TRUE)
		printf("\tChecked %d commands.\n", total_objects);


	/*****************************************/
	/* check all timeperiods...              */
	/*****************************************/
	if(verify_config == TRUE)
		printf("Checking time periods...\n");

	for(temp_timeperiod = timeperiod_list, total_objects = 0; temp_timeperiod != NULL; temp_timeperiod = temp_timeperiod->next, total_objects++) {

		/* check for illegal characters in timeperiod name */
		if(use_precached_objects == FALSE) {
			if(contains_illegal_object_chars(temp_timeperiod->name) == TRUE) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: The name of time period '%s' contains one or more illegal characters.", temp_timeperiod->name);
				errors++;
				}
			}

		/* check for valid timeperiod names in exclusion list */
		for(temp_timeperiodexclusion = temp_timeperiod->exclusions; temp_timeperiodexclusion != NULL; temp_timeperiodexclusion = temp_timeperiodexclusion->next) {

			temp_timeperiod2 = find_timeperiod(temp_timeperiodexclusion->timeperiod_name);
			if(temp_timeperiod2 == NULL) {
				logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Excluded time period '%s' specified in timeperiod '%s' is not defined anywhere!", temp_timeperiodexclusion->timeperiod_name, temp_timeperiod->name);
				errors++;
				}

			/* save the timeperiod pointer for later */
			temp_timeperiodexclusion->timeperiod_ptr = temp_timeperiod2;
			}
		}

	if(verify_config == TRUE)
		printf("\tChecked %d time periods.\n", total_objects);

	/* update warning and error count */
	*w += warnings;
	*e += errors;

	return (errors > 0) ? ERROR : OK;
	}


/* dfs status values */
#define DFS_UNCHECKED                    0  /* default value */
#define DFS_TEMP_CHECKED                 1  /* check just one time */
#define DFS_OK                           2  /* no problem */
#define DFS_NEAR_LOOP                    3  /* has trouble sons */
#define DFS_LOOPY                        4  /* is a part of a loop */

#define dfs_get_status(obj) (obj ? ary[obj->id] : DFS_OK)
#define dfs_set_status(obj, value) ary[obj->id] = (value)

extern struct object_count num_objects;

/**
 * Modified version of Depth-first Search
 * http://en.wikipedia.org/wiki/Depth-first_search
 *
 * In a dependency tree like this (parent->child, dep->dep or whatever):
 * A - B   C
 *      \ /
 *       D
 *      / \
 * E - F - G
 *   /  \\
 * H     H
 *
 * ... we look at the nodes in the following order:
 * A B D C G (marking all of them as OK)
 * E F D G H F (D,G are already OK, E is marked near-loopy F and H are loopy)
 * H (which is already marked as loopy, so we don't follow it)
 *
 * We look at each node at most once per parent, so the algorithm has
 * O(nx) worst-case complexity,, where x is the average number of
 * parents.
 */
/*
 * same as dfs_host_path, but we flip the the tree and traverse it
 * backwards, since core Nagios doesn't need the child pointer at
 * later stages.
 */
static int dfs_servicedep_path(char *ary, service *root, int dep_type, int *errors)
{
	objectlist *olist;
	int status;

	status = dfs_get_status(root);
	if(status != DFS_UNCHECKED)
		return status;

	dfs_set_status(root, DFS_TEMP_CHECKED);

	if (dep_type == NOTIFICATION_DEPENDENCY) {
		olist = root->notify_deps;
	} else {
		olist = root->exec_deps;
	}

	if (!olist) { /* no children, so we can't be loopy */
		dfs_set_status(root, DFS_OK);
		return DFS_OK;
	}
	for (; olist; olist = olist->next) {
		int child_status;
		service *child;
		servicedependency *child_dep = (servicedependency *)olist->object_ptr;

		/* tree is upside down, so look to master */
		child = child_dep->master_service_ptr;
		child_status = dfs_servicedep_path(ary, child, dep_type, errors);

		if (child_status == DFS_OK)
			continue;

		if(child_status == DFS_TEMP_CHECKED) {
			logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Circular %s dependency detected for services '%s;%s' and '%s;%s'\n",
				  dep_type == NOTIFICATION_DEPENDENCY ? "notification" : "execution",
				  root->host_name, root->description,
				  child->host_name, child->description);
			(*errors)++;
			dfs_set_status(child, DFS_LOOPY);
			dfs_set_status(root, DFS_LOOPY);
			continue;
			}
		}

	/*
	 * if we've hit ourself, we'll have marked us as loopy
	 * above, so if we're TEMP_CHECKED still we're ok
	 */
	if (dfs_get_status(root) == DFS_TEMP_CHECKED)
		dfs_set_status(root, DFS_OK);
	return dfs_get_status(root);
}

static int dfs_hostdep_path(char *ary, host *root, int dep_type, int *errors)
{
	objectlist *olist;
	int status;

	status = dfs_get_status(root);
	if(status != DFS_UNCHECKED)
		return status;

	dfs_set_status(root, DFS_TEMP_CHECKED);

	if (dep_type == NOTIFICATION_DEPENDENCY) {
		olist = root->notify_deps;
	} else {
		olist = root->exec_deps;
	}

	for (; olist; olist = olist->next) {
		int child_status;
		host *child;
		hostdependency *child_dep = (hostdependency *)olist->object_ptr;

		child = child_dep->master_host_ptr;
		child_status = dfs_hostdep_path(ary, child, dep_type, errors);

		if (child_status == DFS_OK)
			continue;

		if(child_status == DFS_TEMP_CHECKED) {
			logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: Circular %s dependency detected for hosts '%s' and '%s'\n",
				  dep_type == NOTIFICATION_DEPENDENCY ? "notification" : "execution",
				  root->name, child->name);
			dfs_set_status(child, DFS_LOOPY);
			dfs_set_status(root, DFS_LOOPY);
			(*errors)++;
			continue;
			}
		}

	/*
	 * if we've hit ourself, we'll have marked us as loopy
	 * above, so if we're still TEMP_CHECKED we're ok
	 */
	if (dfs_get_status(root) == DFS_TEMP_CHECKED)
		dfs_set_status(root, DFS_OK);
	return dfs_get_status(root);
}


static int dfs_host_path(char *ary, host *root, int *errors) {
	hostsmember *child = NULL;

	if(dfs_get_status(root) != DFS_UNCHECKED)
		return dfs_get_status(root);

	/* Mark the root temporary checked */
	dfs_set_status(root, DFS_TEMP_CHECKED);

	/* We are scanning the children */
	for(child = root->child_hosts; child != NULL; child = child->next) {
		int child_status = dfs_host_path(ary, child->host_ptr, errors);

		/* we can move on quickly if child is ok */
		if(child_status == DFS_OK)
			continue;

		/* If a child already temporary checked, its a problem,
		 * loop inside, and its a acked status */
		if(child_status == DFS_TEMP_CHECKED) {
			logit(NSLOG_VERIFICATION_ERROR, TRUE, "Error: The hosts '%s' and '%s' form or lead into a circular parent/child chain!", root->name, child->host_ptr->name);
			(*errors)++;
			dfs_set_status(child->host_ptr, DFS_LOOPY);
			dfs_set_status(root, DFS_LOOPY);
			continue;
			}
		}

	/*
	 * If root have been modified, do not set it OK
	 * A node is OK if and only if all of his children are OK
	 * If it does not have child, goes ok
	 */
	if(dfs_get_status(root) == DFS_TEMP_CHECKED)
		dfs_set_status(root, DFS_OK);
	return dfs_get_status(root);
	}

/* check for circular paths and dependencies */
int pre_flight_circular_check(int *w, int *e) {
	host *temp_host = NULL;
	servicedependency *temp_sd = NULL;
	hostdependency *temp_hd = NULL;
	int i, errors = 0;
	unsigned int alloc, dep_type;
	char *ary[2];

	/* bail out if we aren't supposed to verify circular paths */
	if(verify_circular_paths == FALSE)
		return OK;

	/* this would be a valid but pathological case */
	if(num_objects.hosts > num_objects.services)
		alloc = num_objects.hosts;
	else
		alloc = num_objects.services;

	for (i = 0; i < ARRAY_SIZE(ary); i++) {
		if (!(ary[i] = calloc(1, alloc))) {
			while (i) {
				i--;
				my_free(ary[i]);
				}
			logit(NSLOG_CONFIG_ERROR, TRUE, "Error: Unable to allocate memory for circular path checks.\n");
			errors++;
			return ERROR;
			}
		}


	/********************************************/
	/* check for circular paths between hosts   */
	/********************************************/
	if(verify_config == TRUE)
		printf("Checking for circular paths between hosts...\n");

	for(temp_host = host_list; temp_host != NULL; temp_host = temp_host->next) {
		dfs_host_path(ary[0], temp_host, &errors);
		}

	/********************************************/
	/* check for circular dependencies          */
	/********************************************/
	if(verify_config == TRUE)
		printf("Checking for circular host and service dependencies...\n");

	/* check service dependencies */
	/* We must clean the dfs status from previous check */
	for (i = 0; i < ARRAY_SIZE(ary); i++)
		memset(ary[i], 0, alloc);
	for(temp_sd = servicedependency_list; temp_sd != NULL; temp_sd = temp_sd->next) {
		dep_type = temp_sd->dependency_type;
		dfs_servicedep_path(ary[dep_type - 1], temp_sd->dependent_service_ptr, dep_type, &errors);
		}

	/* check host dependencies */
	for (i = 0; i < ARRAY_SIZE(ary); i++)
		memset(ary[i], 0, alloc);
	for(temp_hd = hostdependency_list; temp_hd != NULL; temp_hd = temp_hd->next) {
		dep_type = temp_hd->dependency_type;
		dfs_hostdep_path(ary[dep_type - 1], temp_hd->dependent_host_ptr, dep_type, &errors);
		}

	/* update warning and error count */
	*e += errors;

	for (i = 0; i < ARRAY_SIZE(ary); i++)
		free(ary[i]);

	return (errors > 0) ? ERROR : OK;
	}
