#!/bin/bash
# Serial monitor with auto-reconnect using /dev/ttySERIAL symlink

PORT="/dev/ttySERIAL"
BAUD=9600

echo "Starting serial monitor on $PORT at $BAUD baud"
echo "Press Ctrl+C to exit"
echo "---"

while true; do
  if [ ! -e "$PORT" ]; then
    echo "[$(date '+%H:%M:%S')] Waiting for $PORT..."
    sleep 1
    continue
  fi

  stty -F "$PORT" $BAUD -echo raw 2>/dev/null || {
    echo "[$(date '+%H:%M:%S')] Failed to configure $PORT"
    sleep 1
    continue
  }

  echo "[$(date '+%H:%M:%S')] Connected to $PORT"
  cat "$PORT" 2>/dev/null || {
    echo "[$(date '+%H:%M:%S')] Disconnected or error"
  }

  sleep 1
done
