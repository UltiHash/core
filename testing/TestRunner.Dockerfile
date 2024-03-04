FROM python:3.11.8-bookworm

ENV PYTHONDONTWRITEBYTECODE=1

RUN apt-get update && apt-get install -y sudo

RUN useradd -mU test

COPY requirements.txt /root/requirements.txt

RUN pip install --root-user-action ignore \
    --requirement /root/requirements.txt \
    pytest

COPY TestRunner.entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
