import pytest

def pytest_addoption(parser):
    parser.addoption("--docker-compose-file", dest="docker_compose_file",
                     action="store", required=True)

@pytest.fixture(scope="session")
def docker_compose_file(request):
    return request.config.getoption("--docker-compose-file")

