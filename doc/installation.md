# Installation

UltiHash needs at least 40GB of free storage space. Please make sure that your system can provide them prior to installation.

Visit the [UltiHash](https://www.ultihash.io) webpage and download the zipped installation package by clicking the button [Install](www.example.com).

Fire up a terminal and navigate to the `packaging` directory inside the folder you just unzipped:

```bash
$ cd setup-hardware/packaging
```

Being inside that folder, follow the instructions that apply to your operating system.

## Ubuntu 22.04 {#ubuntu}

 - Ansible: `sudo apt install ansible`

The following commands are used to setup, start and stop UltiHash:

### 1. Setup:

```
$ ansible-playbook --ask-become-pass setup.yml
```
### 2. Start:

```
$ ansible-playbook --ask-become-pass start.yml
```
### 3. Stop:

```
$ ansible-playbook --ask-become-pass stop.yml
```


## Debian:

Make sure you have `sudo` installed and the user you are using has `sudo` privileges.
If this should not be the case, please log in as superuser, either using the root account or the su command, and run

```
# apt-get install sudo
```

Run the following command to add your user account to the sudo group:

```
# usermod -aG sudo [user]
```

Log out of the superuser account and log in with your regular account. Run the command `id` to verify that your user now belongs to the group `sudo`.

From here on, please refer to the steps specified for [Ubuntu](#ubuntu).

## RedHat:

Install all pre-requisites using the dnf packet manager:

```
$ sudo dnf install ansible-core tar bzip2
```

From here on, please refer to the steps specified for [Ubuntu](#ubuntu).

The RedHat instructions should apply for all redhat based distributions, including Red Hat Enterprise Linux (RHEL), CentOS, and Rocky Linux.