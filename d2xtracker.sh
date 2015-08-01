#!/bin/bash
set -e
echo "Starting D2X-XL tracker service."
nohup perl /var/www/web1/html/cgi-bin/d2xtracker.pl > /dev/null &
