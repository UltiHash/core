import os.path

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
    parser.addoption("--data-corpus", action="store", required=False)

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

@pytest.fixture(scope="session")
def files(request):
    if not request.config.getoption("--data-corpus"):
        pytest.skip("skipping the test as data-corpus was not provided")

    files = []
    data_corpus = request.config.getoption("--data-corpus")

    if os.path.exists(data_corpus):
        if os.path.isfile(data_corpus):
            files.append(data_corpus)
        else:
            for root, _, root_files in os.walk(data_corpus):
                for file in root_files:
                    file_path = os.path.join(root, file)
                    files.append(file_path)

            if not files:
                pytest.skip("skipping the test as no files were found")

    else:
        print("invalid path given")
        assert False

    return files

@pytest.fixture
def s3(http_service, aws_secrets):
    return boto3.client('s3',
        **aws_secrets,
        endpoint_url=http_service,
        use_ssl=False)
