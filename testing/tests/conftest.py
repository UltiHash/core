import boto3
import pytest
import requests

from requests.exceptions import ConnectionError

#
# Command line options
#

def pytest_addoption(parser):
    parser.addoption("--cluster-url", action="store", required=True)
    parser.addoption("--aws-access-key-id", action="store", required=True)
    parser.addoption("--aws-secret-access-key", action="store", required=True)

#
# Helper functions
#

def is_responsive(url):
    try:
        response = requests.get(url)
        return True
    except ConnectionError:
        return False

#
# Fixtures
#

@pytest.fixture(scope="session")
def http_service(request):
    return request.config.getoption("--cluster-url")

@pytest.fixture(scope="session")
def aws_secrets(request):
    return {
        "aws_access_key_id": request.config.getoption("--aws-access-key-id"),
        "aws_secret_access_key": request.config.getoption("--aws-secret-access-key")
    }

@pytest.fixture
def s3(http_service, aws_secrets):
    return boto3.client('s3',
        **aws_secrets,
        endpoint_url=http_service,
        use_ssl=False)
