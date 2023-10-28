#!/bin/sh
mkdir -p /tmp/unit-test-blah/state
../build/sbin/unitd --no-daemon \
	--modulesdir ../build/lib/unit/modules \
	--statedir /tmp/unit-test-blah/state \
	--pid /tmp/unit-test-blah/unit.pid \
	--log /tmp/unit-test-blah/unit.log \
	--control unix:/tmp/unit-test-blah/control.unit.sock \
	--tmpdir /tmp/unit-test-blah
