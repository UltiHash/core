 #!/bin/bash

# Get the base path
BASE_DIR=$(dirname "$0")

DOC_FILE_LIST=(
    "${BASE_DIR}/server-db/doc/usage.md"
    "${BASE_DIR}/server-agency/doc/usage.md"
    "${BASE_DIR}/client-shell/doc/usage.md"
    "${BASE_DIR}/client-fuse/doc/usage.md"
)

DOC_FILES=$(IFS=" "; echo "${DOC_FILE_LIST[*]}")

markdown_content="$(cat ${BASE_DIR}/doc/requirements.md)\n"

# Find all .md files in doc folders, excluding the specific file, and exclude lines starting with '%'
markdown_content+=$(grep -h -v '^\s*%' ${DOC_FILES})
echo -e "$markdown_content" | pandoc -N --variable "geometry=margin=1.2in" --variable mainfont="Ubuntu" --variable monofont="Ubuntu Mono" --variable fontsize=12pt --variable version=2.0 --pdf-engine=xelatex --strip-comments --toc --toc-depth=1 -o documentation.pdf
