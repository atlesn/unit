
import re

import pytest
from unit.applications.lang.c import ApplicationC

prerequisites = {'modules': {'c': 'all'}}

client = ApplicationC()


def test_c_application(date_to_sec_epoch, sec_epoch):
    client.load('variables')

    body = 'Test body string.'

    resp = client.post(
        headers={
            'Host': 'localhost',
            'Content-Type': 'text/html',
            'Custom-Header': 'blah',
            'Connection': 'close',
        },
        body=body,
    )

    assert resp['status'] == 200, 'status'
    headers = resp['headers']
    header_server = headers.pop('Server')
    assert re.search(r'Unit/[\d\.]+', header_server), 'server header'
    assert (
        headers.pop('Server-Software') == header_server
    ), 'server software header'

    date = headers.pop('Date')
    assert date[-4:] == ' GMT', 'date header timezone'
    assert abs(date_to_sec_epoch(date) - sec_epoch) < 5, 'date header'

    assert headers == {
        'Connection': 'close',
        'Content-Length': str(len(body)),
        'Content-Type': 'text/html',
        'Request-Method': 'POST',
        'Request-Uri': '/',
        'Http-Host': 'localhost',
        'Server-Protocol': 'HTTP/1.1',
        'Custom-Header': 'blah',
        'Psgi-Version': '11',
        'Psgi-Url-Scheme': 'http',
        'Psgi-Multithread': '',
        'Psgi-Multiprocess': '1',
        'Psgi-Run-Once': '',
        'Psgi-Nonblocking': '',
        'Psgi-Streaming': '1',
    }, 'headers'
    assert resp['body'] == body, 'body'

