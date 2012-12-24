#!/bin/sh
mkdir -p ~/.ssh

cat <<EOF > ~/.ssh/config
Host *
   StrictHostKeyChecking no
   UserKnownHostsFile /dev/null
EOF
