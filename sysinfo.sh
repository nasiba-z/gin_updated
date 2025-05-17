#!/usr/bin/env bash
set -e

echo "=== CPU ==="
lscpu | awk '
  /Socket\(s\)/          { printf("Sockets: %s\n", $2) }
  /^Model name:/         { sub(/Model name:\s*/, ""); printf("Model name: %s\n", $0) }
  /^CPU MHz:/            { sub(/CPU MHz:\s*/, ""); printf("Base frequency (MHz): %s\n", $0) }
  /^CPU\(s\):/           { sub(/CPU(s):\s*/, ""); printf("Logical CPUs: %s\n", $0) }
  /^Core\(s\) per socket:/ { sub(/Core\(s\) per socket:\s*/, ""); printf("Cores/socket: %s\n", $0) }
'

echo
echo "=== MEMORY ==="
# needs sudo if not already root
sudo dmidecode -t memory | awk '
  $1=="Size:" && $2!="No" { print "Module size: "$2" "$3" ×", $4 " slots" }
  $1=="Speed:"        { print "Speed: "$2" "$3 }
  /^Locator:/         { last_loc=$2 }
  $1=="Type:" && $2!="No" { print "Type: "$2, "(", last_loc, ")" }
' | sort -u
echo "Total installed RAM:"
free -h --giga | awk '/^Mem:/ { print $2 " total" }'

echo
echo "=== OS & KERNEL ==="
lsb_release -d | sed 's/Description:/OS:/'
uname -r | awk '{ print "Kernel:", $1 }'

echo
echo "=== COMPILER ==="
gcc --version | head -n1 | awk '{ print "GCC:", $1, $2, $3 }'