#!/bin/bash
#
# Script unshares network namespace and runs bash as $USER
# Unf, I don't know how to simply avoid working directory change
# (may change bashrc and undo changes after, but whatever)

if [[ -z $1 ]]; then
	echo "USAGE: ./netnssh.sh NETWORK_NS_NAME"
	exit 1
fi

echo "Unsharing to [$1] netns..." && \
sudo ip netns exec $1 su - $USER
