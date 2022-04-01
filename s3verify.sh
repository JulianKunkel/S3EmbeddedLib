#!/bin/bash
# set -eux 
# inspired from the https://github.com/minio/mint tests used to run correctness, benchmarking, and stress tests 
# against an S3 compatible API. We will contribute this script to the mentioned project when the time is right :)

export MINT_DATA_DIR=$PWD/my-mint-dir
export MINT_MODE=core

export S3_ACCESS_KEY_ID="Q3AM3UQ867SPQQA43P2F"
export S3_SECRET_ACCESS_KEY="zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG"
# export S3_HOSTNAME="play.minio.io:9000"
# export S3_HOSTNAME="s3.wasabisys.com"

output_log_file="outs3.log"
error_log_file="errors3.log"

# # run tests
"$PWD"/mintests3.sh  1>>"$output_log_file" 2>"$error_log_file"