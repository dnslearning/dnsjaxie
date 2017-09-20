#!/bin/sh

cd /home/dnsjaxie && flock -n .lock -c './dnsjaxie -f shard.conf >> logs/cron.log 2>> logs/cron.log.err'
