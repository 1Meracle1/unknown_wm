set -e

XEPHYR=$(whereis -b Xephyr | sed -E 's/^.*: ?//')
if [ -z "$XEPHYR" ]; then
  echo "Xephyr not found"
  exit 1
fi

xinit ./xinitrc -- "$XEPHYR" :100 -ac -screen 800x600 -host-cursor
