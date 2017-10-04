#!/bin/sh

cd /home/dnsjaxie && flock -n .lock -c './dnsjaxie -f shard.conf 2>&1 | ts >> logs/cron.log'
