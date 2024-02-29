# UltiHash Feature Testing

This folder contains feature aka. end-to-end tests for UltiHash. They are used
to verify application features and stability during pull requests checks. They
also can be used to debug issues with the application.


# Starting Tests

Calling `start-e2e.sh` will build the whole application, start the cluster in a
docker-compose setup and run several test suites against it.

`start-e2e.sh` is a wrapper around a collection of smaller scripts that offer
more fine-grained control over the testing process. All of them are located under
`scripts` folder:

- `uh-build-container.sh` build the application container image that is used in
  the compose setup
- `uh-build-testrunner.sh` build the container image used to run the pytest test
  cases
- `uh-container-start.sh` start the docker compose cluster
- `uh-container-stop.sh` stop the docker compose cluster
- `uh-debug-service.sh` start a gdbserver in a selected container
- `uh-run-tests-ceph.sh` run the Ceph S3 test suite against a running cluster
- `uh-run-tests-ulti.sh` run the UltiHash test suite against a running cluster

Access to these scripts can be activated by sourcing `activate` into your running
shell:

```
$> . testing/activate
```

will add the scripts directory to the path and configure required environment
variables.
