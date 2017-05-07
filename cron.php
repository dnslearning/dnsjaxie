<?php

$lockFile = __DIR__.'/lock.pid';
$pid = @file_get_contents($lockFile);

if ($pid > 0 && file_exists("/proc/$pid")) {
  echo "Already running\n";
  return;
}

$pid = getmypid();
file_put_contents($lockFile, $pid);
passthru("./dnsjaxie -f shard.conf");
unlink($lockFile);
