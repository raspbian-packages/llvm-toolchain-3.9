#!/bin/bash -e
$(dirname "$0")/config.guess | sed -r s/arm[a-zA-Z0-9]*/armv6/
