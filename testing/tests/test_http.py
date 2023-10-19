#
# This test case connects to entry point requesting a non-existant file and
# checks whether an error code is returned.
#

import pytest
import requests

from requests.exceptions import ConnectionError

def is_responsive(url):
    try:
        response = requests.get(url)
        return True
    except ConnectionError:
        return False

@pytest.fixture(scope="session")
def http_service(docker_ip, docker_services):
    port = docker_services.port_for("en", 8080)
    url = "http://{}:{}".format(docker_ip, port)
    docker_services.wait_until_responsive(
        timeout=30.0, pause=0.1, check=lambda: is_responsive(url)
        )
    return url

def test_http_error(http_service):
    response = requests.get(http_service + "/non-existent-bucket")

    assert response.status_code != 200

