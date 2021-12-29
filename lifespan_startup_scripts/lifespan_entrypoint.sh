#!/bin/bash
python3 /home/image_server/entrypoint.py
tail -f /dev/null
# httpd-foreground
# ns_image_server