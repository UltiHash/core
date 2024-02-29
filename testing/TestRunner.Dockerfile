FROM python:3.11.8-bookworm

COPY requirements.txt /root/requirements.txt

RUN pip install --root-user-action ignore \
    --requirement /root/requirements.txt \
    pytest
