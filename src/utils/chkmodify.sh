#!/usr/bin/env bash

# if some files in this directory were modified,then
# return 1 and continue to compile,otherwise return 0

# CALL_NAME=$0
# FILE_NAME=$(basename ${CALL_NAME})
# REAL_PATH=$(realpath ${CALL_NAME})
# WORK_PATH=${REAL_PATH%${FILE_NAME}}

WORK_PATH=$1

if ! [[ -n ${WORK_PATH} ]]; then
    exit 1
fi
# check if the first parameter is valid

ROOT_PATH=$(realpath ${WORK_PATH})
LOCK_FILE=${WORK_PATH}/.proj.status_lock
OUTPUT_FILE=${WORK_PATH}/.proj.status_log
# get some basic info before start main code

export curr
export hist

function get_current {
    curr=$(LANG= git diff ${ROOT_PATH} | sha256sum | awk '{print $1}')
}

function get_history {
    if [[ -f ${OUTPUT_FILE} ]]; then
        source ${OUTPUT_FILE}
    else
        return 1
    fi
}

function update_history {
    echo "#!/usr/bin/env bash" >${OUTPUT_FILE}
    echo "export hist=${curr}" >>${OUTPUT_FILE}
}

function locked {
    if [[ -f ${LOCK_FILE} ]]; then
        return 0
    fi

    return 1
}
# the lock file is used to mark if the last process which compiled this module was done.
# If the process was not finished successfully,the lock file will prevent this script from running.
# At the end of the process,the lock file will be removed

function lock {
    echo >${LOCK_FILE}
}

if locked; then
    exit 1
    # return and continue to compile modules till the compiling task is finished
fi

get_current

if ! get_history; then
    update_history
fi

lock

if [[ ${curr} == ${hist} ]]; then
    exit 0
else
    update_history
    exit 1
    # if they are not the same value,then update and exit 1,
    # that means some files in this directory may be modified.
fi

