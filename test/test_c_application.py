
import re
import time

import pytest
from unit.applications.lang.c import ApplicationC

prerequisites = {'modules': {'c': 'all'}}

client = ApplicationC()


def test_c_application(date_to_sec_epoch, sec_epoch):
    client.load('request')

    # Allow compiler to complete.
    time.sleep(0.5)

    resp = client.get(
        headers={
            'Host': 'localhost',
            'Content-Type': 'text/html',
            'Connection': 'close',
        }
    )

    assert resp['status'] == 200, 'status'

