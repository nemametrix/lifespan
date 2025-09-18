import os
import sys
from collections import OrderedDict
from pathlib import Path


CONFIG_ROOT = Path("/usr/local/etc")
APACHE_HTTPD_CONF = Path("/etc/apache2/httpd.conf")
PHP_INI = Path("/etc/php.ini")


def _env(name: str, default: str = "") -> str:
    """Return environment variable value or a fallback as a string."""
    value = os.getenv(name, default)
    return str(value)


def build_ns_image_server_config() -> OrderedDict:
    config = OrderedDict([
        ("host_name", _env("host_name")),
        ("long_term_storage_directory", _env("long_term_storage_directory")),
        ("results_storage_directory", _env("results_storage_directory")),
        ("volatile_storage_directory", _env("volatile_storage_directory")),
        ("central_sql_hostname", _env("central_sql_hostname")),
        ("central_sql_username", _env("central_sql_username")),
        ("central_sql_password", _env("central_sql_password")),
        ("central_sql_databases", _env("central_sql_databases")),
        ("local_buffer_sql_hostname", _env("local_buffer_sql_hostname")),
        ("local_buffer_sql_username", _env("local_buffer_sql_username")),
        ("local_buffer_sql_database", _env("local_buffer_sql_database")),
        ("local_buffer_sql_password", _env("local_buffer_sql_password")),
        ("act_as_image_capture_server", _env("act_as_image_capture_server")),
        ("device_capture_command", "/usr/local/bin/scanimage"),
        ("device_list_command", "/usr/local/bin/sane-find-scanner"),
        ("device_barcode_coordinates", "-l 0in -t 10.3in -x 8in -y 2in"),
        ("simulated_device_name", "."),
        ("device_names", ""),
        ("output_files_with_all_read_permissions", "yes"),
        ("allow_multiple_processes_per_system", _env("allow_multiple_processes_per_system", "no")),
        ("number_of_times_to_check_empty_processing_job_queue_before_stopping", 0),
        ("act_as_processing_node", _env("act_as_processing_node", "yes")),
        ("nodes_per_machine", _env("nodes_per_machine", "1")),
        ("hide_window", "no"),
        ("compile_videos", "yes"),
        ("video_compiler_filename", "/usr/bin/x264"),
        ("video_ppt_compiler_filename", "/usr/bin/ffmpeg"),
        ("halt_on_new_software_release", "no"),
        ("latest_release_path", "image_server_software/image_server_win32.exe"),
        ("run_autonomously", "yes"),
        ("verbose_debug_output", "true"),
        ("dispatcher_refresh_interval", 6000),
        ("mail_path", ""),
        ("mail_from", "Local User <user@localhost>"),
        ("ethernet_interface", "eth0"),
        ("dispatcher_port", 1043),
        ("server_crash_daemon_port", 1042),
        ("server_timeout_interval", 300),
        ("log_filename", "image_server_log.txt"),
        ("maximum_memory_allocation_in_mb", 3840),
        ("verbose_local_storage_space_reporting", "false"),
    ])

    optional_overrides = OrderedDict([
        ("contact_name", os.getenv("contact_name")),
        ("contact_email", os.getenv("contact_email")),
        ("contact_phone", os.getenv("contact_phone")),
        ("default_database", os.getenv("default_database")),
    ])

    for key, value in optional_overrides.items():
        if value:
            config[key] = value

    return config


def build_ns_worm_browser_config() -> OrderedDict:
    return OrderedDict([
        ("max_width", 1024),
        ("max_height", 768),
        ("hand_annotation_resize_factor", 2),
        ("mask_upload_database", _env("mask_upload_database")),
        ("mask_upload_hostname", _env("mask_upload_hostname")),
        ("verbose_debug_output", _env("verbose_debug_output")),
        ("window_scale_factor", 1),
    ])


def write_ini(path: Path, config: OrderedDict) -> None:
    path = CONFIG_ROOT / path
    tmp_path = path.with_suffix(path.suffix + ".tmp")
    path.parent.mkdir(parents=True, exist_ok=True)
    with tmp_path.open("w", encoding="utf-8") as handle:
        for key, value in config.items():
            handle.write(f"{key} = {value}\n")
    tmp_path.replace(path)
    print(f"[lifespan] updated {path}")


def append_server_configuration(host_name: str) -> None:
    APACHE_HTTPD_CONF.parent.mkdir(parents=True, exist_ok=True)
    with APACHE_HTTPD_CONF.open("a", encoding="utf-8") as httpd_conf:
        httpd_conf.write("\n")
        httpd_conf.write(f"ServerName {host_name}")
        httpd_conf.write("\n")
        httpd_conf.write("Options FollowSymLinks")


def set_php_timezone(timezone: str) -> None:
    with PHP_INI.open("a", encoding="utf-8") as php_file:
        php_file.write("\n")
        php_file.write(f"date.timezone = {timezone}")


def main() -> None:
    try:
        write_ini(Path("ns_image_server.ini"), build_ns_image_server_config())
        write_ini(Path("ns_worm_browser.ini"), build_ns_worm_browser_config())

        append_server_configuration(_env("host_name"))
        set_php_timezone(_env("timezone", "America/Los_Angeles"))
    except Exception as exc:  # pragma: no cover - defensive for container startup
        print(f"[lifespan] failed to render configuration: {exc}", file=sys.stderr)
        raise


if __name__ == "__main__":
    main()
