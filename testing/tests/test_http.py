#
# This test case connects to entry point requesting a non-existant file and
# checks whether an error code is returned.
#

import requests

def test_http_error(http_service):
    response = requests.get(http_service + "/non-existent-bucket")

    assert response.status_code != 200
