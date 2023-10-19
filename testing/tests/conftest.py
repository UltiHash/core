import boto3
import pytest
import requests

from requests.exceptions import ConnectionError


def pytest_addoption(parser):
    parser.addoption("--docker-compose-file", action="store", required=True)
    parser.addoption("--aws-access-key-id", action="store", required=True)
    parser.addoption("--aws-secret-access-key", action="store", required=True)

@pytest.fixture(scope="session")
def aws_secrets(request):
    return {
        "aws_access_key_id": request.config.getoption("--aws-access-key-id"),
        "aws_secret_access_key": request.config.getoption("--aws-secret-access-key")
    }

@pytest.fixture(scope="session")
def docker_compose_file(request):
    return request.config.getoption("--docker-compose-file")

@pytest.fixture
def s3(http_service, aws_secrets):
    return boto3.client('s3',
        **aws_secrets,
        endpoint_url=http_service,
        use_ssl=False)

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

