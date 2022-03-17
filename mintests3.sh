#!/bin/bash
#  set -eux
# inspired from the https://github.com/minio/mint tests used to run correctness, benchmarking, and stress tests 
# against an S3 compatible API. We will contribute this script to the mentioned project when the time is right :)

if [ -n "$MINT_MODE" ]; then
    if [ -z "${MINT_DATA_DIR+x}" ]; then
        echo "MINT_DATA_DIR not defined"
        exit 1
    fi
    if [ -z "${S3_HOSTNAME+x}" ]; then
        echo "SERVER_ENDPOINT not defined"
        exit 1
    fi
    if [ -z "$S3_ACCESS_KEY_ID" ]; then
        echo "S3_ACCESS_KEY_ID required"
        exit 1;
    fi

    if [ -z "$S3_SECRET_ACCESS_KEY" ]; then
        echo "S3_SECRET_ACCESS_KEY required"
        exit 1;
    fi
fi

# if [ -z "${SERVER_ENDPOINT+x}" ]; then
#     SERVER_ENDPOINT="play.minio.io:9000"
#     ACCESS_KEY="Q3AM3UQ867SPQQA43P2F"
#     SECRET_KEY="zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG"
#     ENABLE_HTTPS=1
#     SERVER_REGION="us-east-1"
# fi

WORK_DIR="$PWD"
DATA_DIR="$MINT_DATA_DIR"
if [ -z "$MINT_MODE" ]; then
    WORK_DIR="$PWD/.run-$RANDOM"
    DATA_DIR="$WORK_DIR/data"
fi
mkdir -p "$DATA_DIR"
FILE_1_MB="$DATA_DIR/datafile-1-MB"
FILE_65_MB="$DATA_DIR/datafile-65-MB"
dd if=/dev/urandom of="$FILE_1_MB" bs=1024k count=1
dd if=/dev/urandom of="$FILE_65_MB" bs=1024k count=65

declare FILE_1_MB_MD5SUM
declare FILE_65_MB_MD5SUM

BUCKET_NAME="s3-test-bucket-$RANDOM"
S3_CMD=$(command -v s3)

function get_md5sum()
{
    filename="$FILE_1_MB"
    out=$(md5sum "$filename" 2>/dev/null)
    rv=$?
    if [ "$rv" -eq 0 ]; then
        awk '{ print $1 }' <<< "$out"
    fi

    return "$rv"
}

function get_time()
{
    date +%s%N
}

function get_duration()
{
    start_time=$1
    end_time=$(get_time)

    echo $(( (end_time - start_time) / 1000000 ))
}

function log_success()
{
    if [ -n "$MINT_MODE" ]; then
        printf '{"name": "s3", "duration": "%d", "function": "%s", "status": "PASS"}\n' "$(get_duration "$1")" "$2"
    fi
}

function show()
{
    if [ -z "$MINT_MODE" ]; then
        func_name="$1"
        echo "Running $func_name()"
    fi
}

function fail()
{
    rv="$1"
    shift

    if [ "$rv" -ne 0 ]; then
        echo "$@"
    fi

    return "$rv"
}

function assert()
{
    expected_rv="$1"
    shift
    start_time="$1"
    shift
    func_name="$1"
    shift

    err=$("$@" 2>&1)
    rv=$?
    if [ "$rv" -ne 0 ] && [ "$expected_rv" -eq 0 ]; then
        if [ -n "$MINT_MODE" ]; then
            err=$(printf '%s' "$err" | python -c 'import sys,json; print(json.dumps(sys.stdin.read()))')
            ## err is already JSON string, no need to double quote
            printf '{"name": "s3", "duration": "%d", "function": "%s", "status": "FAIL", "error": %s}\n' "$(get_duration "$start_time")" "$func_name" "$err"
        else
            echo "s3: $func_name: $err"
        fi

        exit "$rv"
    fi

    return 0
}

function assert_success() {
    assert 0 "$@"
}

function assert_failure() {
    assert 1 "$@"
}

function s3_cmd()
{
    cmd=( "${S3_CMD}" "$@" )
    "${cmd[@]}"
    rv=$?
    return "$rv"
}

function s3_sync()
{   DATA_DIR=$1
    BUCKET_NAME=$2
    while IFS= read -r -d '' file; do
    filename=$(basename "$file")
    s3 put "${BUCKET_NAME}/${filename}" filename="$file"
    done < <(find "$DATA_DIR" -maxdepth 1 -type f -print0)
}

function check_md5sum()
{
    expected_checksum="$1"
    shift
    filename="$*"

    checksum="$(get_md5sum "$filename")"
    rv=$?
    if [ "$rv" -ne 0 ]; then
        echo "unable to get md5sum for $filename"
        return "$rv"
    fi

    if [ "$checksum" != "$expected_checksum" ]; then
        echo "$filename: md5sum mismatch"
        return 1
    fi

    return 0
}

function test_make_bucket()
{
    show "${FUNCNAME[0]}"

    start_time=$(get_time)
    bucket_name="s3-test-bucket-$RANDOM"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd create "${bucket_name}"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd delete "${bucket_name}"

    log_success "$start_time" "${FUNCNAME[0]}"
}

function test_make_bucket_error() {
    show "${FUNCNAME[0]}"

    start_time=$(get_time)
    bucket_name="s3-test%bucket%$RANDOM"
    assert_failure "$start_time" "${FUNCNAME[0]}" s3_cmd create "${bucket_name}"

    log_success "$start_time" "${FUNCNAME[0]}"
}

function setup()
{
    start_time=$(get_time)
    bucket_name="s3-test-bucket-$RANDOM"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd create "${bucket_name}"
}

function teardown()
{
    start_time=$(get_time)
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd delete --force "${bucket_name}"
    rm -f "$FILE_1_MB" "$FILE_65_MB"
}

function test_put_object()
{
    show "${FUNCNAME[0]}"

    start_time=$(get_time)
    object_name="s3-test-object-$RANDOM"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd put "${bucket_name}/${object_name}" filename="${FILE_1_MB}"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd delete "${bucket_name}/${object_name}"

    log_success "$start_time" "${FUNCNAME[0]}"
}

function test_put_object_error()
{
    show "${FUNCNAME[0]}"
    start_time=$(get_time)

    object_long_name=$(printf "s3-test-object-%01100d" 1)
    assert_failure "$start_time" "${FUNCNAME[0]}" s3_cmd put "${bucket_name}/${object_long_name}" filename="${FILE_1_MB}"

    log_success "$start_time" "${FUNCNAME[0]}"
}

function test_put_object_multipart()
{
    show "${FUNCNAME[0]}"

    start_time=$(get_time)
    object_name="s3-test-object-$RANDOM"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd put "${bucket_name}/${object_name}" filename="${FILE_65_MB}"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd delete "${bucket_name}/${object_name}"

    log_success "$start_time" "${FUNCNAME[0]}"
}

function test_get_object()
{
    show "${FUNCNAME[0]}"

    start_time=$(get_time)
    object_name="s3-test-object-$RANDOM"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd put "${bucket_name}/${object_name}" filename="${FILE_1_MB}"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd get "${bucket_name}/${object_name}" filename="${object_name}.downloaded"
    assert_success "$start_time" "${FUNCNAME[0]}" check_md5sum "$FILE_1_MB_MD5SUM" "${object_name}.downloaded"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd delete "${bucket_name}/${object_name}"
    assert_success "$start_time" "${FUNCNAME[0]}" rm -f "${object_name}.downloaded"

    log_success "$start_time" "${FUNCNAME[0]}"
}

function test_get_object_error()
{
    show "${FUNCNAME[0]}"

    start_time=$(get_time)
    object_name="s3-test-object-$RANDOM"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd put "${bucket_name}/${object_name}" filename="${FILE_1_MB}"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd delete "${bucket_name}/${object_name}"
    assert_failure "$start_time" "${FUNCNAME[0]}" s3_cmd get "${bucket_name}/${object_name}" filename="${object_name}.downloaded"
    assert_success "$start_time" "${FUNCNAME[0]}" rm -f "${object_name}.downloaded"

    log_success "$start_time" "${FUNCNAME[0]}"
}

function test_get_object_multipart()
{
    show "${FUNCNAME[0]}"

    start_time=$(get_time)
    object_name="s3-test-object-$RANDOM"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd put "${bucket_name}/${object_name}" filename="${FILE_65_MB}"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd get "${bucket_name}/${object_name}" filename="${object_name}.downloaded"
    assert_success "$start_time" "${FUNCNAME[0]}" check_md5sum "$FILE_65_MB_MD5SUM" "${object_name}.downloaded"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd delete "${bucket_name}/${object_name}"
    assert_success "$start_time" "${FUNCNAME[0]}" rm -f "${object_name}.downloaded"

    log_success "$start_time" "${FUNCNAME[0]}"
}

function test_sync_list_objects()
{
    show "${FUNCNAME[0]}"

    start_time=$(get_time)
    bucket_name="s3-test-bucket-$RANDOM"
    object_name="s3-test-object-$RANDOM"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd create "${bucket_name}"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_sync "$DATA_DIR" "${bucket_name}"

    diff -bB <(ls "$DATA_DIR") <("${S3_CMD}" list "${bucket_name}" | awk 'NR>2' | awk '{print $1}') >/dev/null 2>&1
    assert_success "$start_time" "${FUNCNAME[0]}" fail $? "sync and list differs"
    assert_success "$start_time" "${FUNCNAME[0]}" s3_cmd delete --force "${bucket_name}"

    log_success "$start_time" "${FUNCNAME[0]}"
}

function run_test()
{
    test_make_bucket
    test_make_bucket_error

    setup

    test_put_object
    test_put_object_error
    test_put_object_multipart
    test_get_object
    test_get_object_multipart
    test_sync_list_objects

    teardown
}

function __init__()
{
    set -e
    # For Mint, setup is already done.  For others, setup the environment
    if [ -z "$MINT_MODE" ]; then
        mkdir -p "$WORK_DIR"
        mkdir -p "$DATA_DIR"

        # If s3 executable binary is not available in current directory, use it in the path.
        if [ ! -x "$S3_CMD" ]; then
            echo "'s3' executable binary not found in current directory and in path"
            exit 1
        fi
    fi

    if [ ! -x "$S3_CMD" ]; then
        echo "$S3_CMD executable binary not found"
        exit 1
    fi

    if [ ! -e "$FILE_1_MB" ]; then
        shred -n 1 -s 1MB - >"$FILE_1_MB"
    fi

    if [ ! -e "$FILE_65_MB" ]; then
        shred -n 1 -s 65MB - >"$FILE_65_MB"
    fi

    set -E
    set -o pipefail

    FILE_1_MB_MD5SUM="$(get_md5sum "$FILE_1_MB")"
    rv=$?
    if [ $rv -ne 0 ]; then
        echo "unable to get md5sum of $FILE_1_MB"
        exit 1
    fi

    FILE_65_MB_MD5SUM="$(get_md5sum "$FILE_65_MB")"
    rv=$?
    if [ $rv -ne 0 ]; then
        echo "unable to get md5sum of $FILE_65_MB"
        exit 1
    fi

    set +e
}

function main()
{
    ( run_test )
    rv=$?

    if [ -z "$MINT_MODE" ]; then
        rm -fr "$WORK_DIR" "$DATA_DIR"
    fi

    exit "$rv"
}

__init__ "$@"
main "$@"
