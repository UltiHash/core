# To use this settings, please run the following command once.
# echo 'add-auto-load-safe-path <project-path>/.gdbinit' >> /home/sungsik/.config/gdb/gdbinit

# If you experiencing SEGFAULT,
# 1. Don't forget to run `ulimit -c unlimited` before running the program.
# 2. Run gdb <executable> `ls -t /var/lib/apport/coredump/core.* | head -n 1`

set print pretty on
