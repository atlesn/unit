#!/bin/sh

curl -XPUT --data-binary @config.json --unix-socket /tmp/unit-test-blah/control.unit.sock http://localhost/config/
