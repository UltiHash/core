# Overview

UltiHash upload script. Upload data recursively to UltiHash cluster using boto3 to access the S3 API.


# Requirements

- Python 3
- boto3 library installed

# Installation

Create a virtual environment to hold the Python libraries:

```
$> python -m venv venv
$> . venv/bin/activate
```

Install all requirements:

```
$> pip install --requirement requirements.txt
```

# Usage

Upload a folder to a UH cluster running locally:

```
$> ./uh_upload.py $PATH_TO_FOLDER
```

Upload a folder to a given S3 instance

```
$> ./uh_upload.py --url http://uh.my-domain.net $PATH_TO_FOLDER
```

Override the default target bucket:

```
$> ./uh_upload.py --bucket MY_BUCKET $PATH_TO_FOLDER
```
