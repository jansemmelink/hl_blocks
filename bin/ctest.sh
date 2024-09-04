#!/bin/bash

function debug() {
    echo -e $(date "+%Y-%m-%d") DEBUG $* >&2
}

function error() {
    echo -e $(date "+%Y-%m-%d") ERROR $* >&2
    exit 1
}

debug "Getting test names ..."
test_names=$(mktemp)
grep -hoe "^TEST(.*)" test_*.c | sed "s/TEST(\(.*\))/test_r_\1/" > ${test_names}

debug "Got $(wc -l ${test_names} | awk '{print $1}') test names"

test_main="generated_main_test_program.c"
cat << EOF > ${test_main}
#include <stdio.h>
#include "error_stack.h"

// include test files:
EOF

ls -1 test_*.c | sed "s/^/#include \"/;s/\$/\"/" >> ${test_main}

cat << EOF >> ${test_main}

// main test function to run all tests
int main() {
    //calling all tests:
EOF

# call all the tests
cat ${test_names} | \
awk '{print "    printf(\"\\n\\n===== TEST: " $1 " ======\\n\");\n\
    if (" $1 "() != 0)\n    {\n\
        printf (\"" $1 " FAILED.\\n\");\n\
        error_stack_r_print (stderr);\n\
        exit (1);\n\
    } else {\n\
        printf (\"" $1 " PASSED.\\n\");\n\
    }\n"\
}' >> ${test_main}

cat << EOF >> ${test_main}
    return SUCCESS();
}/*main*/
EOF

# debug "Generated:"
# cat ${test_main}

#rm -f ${test_main}
rm -f ${test_names}

debug "Compiling..."
gcc *.c -o test_main || error "Failed to compile"

debug "Running..."
./test_main || error "Tests failed"
debug "PASSED"
exit 0