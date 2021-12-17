import os
import mysql.connector
import subprocess


ns_image_server = {
'host_name' : os.getenv('host_name',''),
'long_term_storage_directory' : os.getenv('long_term_storage_directory',''),
'results_storage_directory' : os.getenv('results_storage_directory',''),
'volatile_storage_directory' : os.getenv('volatile_storage_directory',''),
'central_sql_hostname' : os.getenv('central_sql_hostname',''),
'central_sql_username' : os.getenv('central_sql_username',''),
'central_sql_password' : os.getenv('central_sql_password',''),
'central_sql_databases' : os.getenv('central_sql_databases',''),
'local_buffer_sql_hostname' : os.getenv('local_buffer_sql_hostname',''),
'local_buffer_sql_username' : os.getenv('local_buffer_sql_username',''),
'local_buffer_sql_database' : os.getenv('local_buffer_sql_database',''),
'local_buffer_sql_password' : os.getenv('local_buffer_sql_password',''),
'act_as_image_capture_server' : os.getenv('act_as_image_capture_server',''),
'device_capture_command' : "/usr/local/bin/scanimage",
'device_list_command' : "/usr/local/bin/sane-find-scanner",
'device_barcode_coordinates' : "-l 0in -t 10.3in -x 8in -y 2in",
'simulated_device_name' : '.',
'device_names' : '',
'output_files_with_all_read_permissions' : 'yes',
'allow_multiple_processes_per_system' : 'yes',
'number_of_times_to_check_empty_processing_job_queue_before_stopping' : 0,
'act_as_processing_node' : os.getenv('act_as_processing_node','yes'),
'nodes_per_machine' : os.getenv('nodes_per_machine','1'),
'hide_window' : 'no',
'compile_videos' : 'yes',
'video_compiler_filename' : '/usr/bin/x264',
'video_ppt_compiler_filename' : '/usr/bin/ffmpeg',
'halt_on_new_software_release' : 'no',
'latest_release_path' : 'image_server_software/image_server_win32.exe',
'run_autonomously' : 'yes',
'verbose_debug_output' : 'true',
'dispatcher_refresh_interval' : 6000,
'mail_path' : '',#'/bin/mail',
'mail_from' : 'Local User <user@localhost>',
'ethernet_interface' : 'eth0',
'dispatcher_port' : 1043,
'server_crash_daemon_port' : 1042,
'server_timeout_interval' : 300,
'log_filename' : 'image_server_log.txt',
'maximum_memory_allocation_in_mb' : 3840,
'verbose_local_storage_space_reporting' : 'false'
}


ns_worm_browser = {
'max_width' : 1024,
'max_height' : 768,
'hand_annotation_resize_factor' : 2,
'mask_upload_database' : os.getenv('mask_upload_database',''),
'mask_upload_hostname' : os.getenv('mask_upload_hostname',''),
'verbose_debug_output' : os.getenv('verbose_debug_output',''),
'window_scale_factor' : 1
}


timezone = os.getenv("timezone","America/Los_Angeles")

with open('/usr/local/etc/ns_image_server.ini','w+') as ns_image_server_file:
    for key,value in ns_image_server.items():
        ns_image_server_file.write(key+" = "+str(value)+"\n")

with open('/usr/local/etc/ns_worm_browser.ini','w+') as ns_worm_browser_file:
    for key,value in ns_worm_browser.items():
        ns_worm_browser_file.write(key+" = "+str(value)+"\n")


with open('/etc/apache2/httpd.conf','a') as httpd_conf:
    httpd_conf.write("\n")
    httpd_conf.write("ServerName "+ ns_image_server['host_name'])
    httpd_conf.write("\n")
    httpd_conf.write("Options FollowSymLinks")

with open("/etc/php.ini",'a') as php_file:
    php_file.write("\n")
    php_file.write("date.timezone = " + timezone)



# lifespan_db = mysql.connector.connect(
#   host=ns_image_server['central_sql_hostname'],
#   user=ns_image_server['central_sql_username'],
#   passwd=ns_image_server['central_sql_password']
# )


# database = ns_image_server['central_sql_databases'].split(',')[0]

# query = """
# SELECT *
# FROM information_schema.tables
# WHERE table_schema = '{}'
#     AND table_name = 'worm_detection_results'
# LIMIT 1;
# """.format(database)

# cursor = lifespan_db.cursor()
# cursor.execute(query)
# result = cursor.fetchall()

# if len(result) == 0:
#     subprocess.Popen(["httpd"])
#     configure_db = subprocess.Popen(["ns_image_server"])
#     print(configure_db.communicate())
