#!/bin/bash
set -euo pipefail

python3 /home/image_server/entrypoint.py

exec tail -f /dev/null
# httpd-foreground
# ns_image_server
