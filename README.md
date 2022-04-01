# S3EmbeddedLib
This repository contains a series of pseudo implementations of [LibS3 ](https://github.com/bji/libs3) that can be used as a drop-in replacement of LibS3, i.e., it can be linked together with applications that already use LibS3.
The purpose is to allow convenient testing of the S3 API usage and evaluate performance implications when changing or optimizing some components of the S3 software stack.

It requires the original header of the libS3.h.
During the linking process, any program can be linked with this library.

Any program compiled using the LibS3 library can be dynamically linked to this library as well.

The current implementations depend on a globally accessible shared file system to store the data.

The following libraries are provided:
  * libS3e.so: This is an embedded library wrapper that converts libS3 calls to POSIX calls.
  * libS3r.so: This library converts the libS3 calls via a binary conversion to TCP calls to a local libS3-gw application that then executes these POSIX calls.

## Options
At runtime, specify the directory path as the hostname in S3_initialize.

## Usage

Compile the library:
```console
$ wget https://raw.githubusercontent.com/bji/libs3/master/inc/libs3.h
$ make
```
The simplest way to use this library is to dynamically link your LibS3 client program to it; using symlinks you can easily switch between LibS3 and S3Embedded:

```console
$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/foo/ur-git/S3EmbeddedLib
$ ln -sf $PWD/libS3e.so libs3.so.4  # use S3Embedded local
$ ln -sf $PWD/libS3r.so libs3.so.4  # use S3Embedded gateway
$ ln -sf $LIBS3_PATH/libs3.so.4 libs3.so.4  # use the libs3 library
```

### Usage with IOR

To build [IOR](https://github.com/hpc/ior) with the pseudo library, the linking process can be changed as follows:
```console
$ rm md-workbench mdtest ior
$ make V=1 extraLDADD="/home/kunkel/ur-git/ior/ior-master/S3EmbeddedLib/libS3e.so"
```
You can link the embedded library libS3e.so or the remote library libS3r.so.
You may also create a symlink to any library as libS3.so and then change it on the fly.


## S3 compatibility tests
The s3verify.sh script is inspired from the [Mint](https://github.com/minio/mint) tests used to run correctness, benchmarking, and stress tests against an S3 compatible API.
```console
# Against MinIO Test Endpoint using the original LibS3
$ export S3_HOSTNAME="play.minio.io:9000"
$ ./s3verify.sh
$ cat outs3.log
{"name": "s3", "duration": "1612", "function": "test_make_bucket", "status": "PASS"}
{"name": "s3", "duration": "189", "function": "test_make_bucket_error", "status": "PASS"}
{"name": "s3", "duration": "22012", "function": "test_put_object", "status": "PASS"}
{"name": "s3", "duration": "394", "function": "test_put_object_error", "status": "PASS"}
{"name": "s3", "duration": "185532", "function": "test_put_object_multipart", "status": "PASS"}
{"name": "s3", "duration": "26369", "function": "test_get_object", "status": "PASS"}
{"name": "s3", "duration": "327153", "function": "test_get_object_multipart", "status": "PASS"}
# Against S3Embedded with tmpfs as Storage Backend
$ ln -sf $PWD/libS3e.so libs3.so.4
$ export S3_HOSTNAME="/dev/shm/"
$ ./s3verify.sh
$ cat outs3.log
{"name": "s3", "duration": "145", "function": "test_make_bucket", "status": "PASS"}
{"name": "s3", "duration": "40", "function": "test_make_bucket_error", "status": "PASS"}
{"name": "s3", "duration": "83", "function": "test_put_object", "status": "PASS"}
{"name": "s3", "duration": "46", "function": "test_put_object_error", "status": "PASS"}
{"name": "s3", "duration": "551", "function": "test_put_object_multipart", "status": "PASS"}
{"name": "s3", "duration": "309", "function": "test_get_object", "status": "PASS"}
{"name": "s3", "duration": "753", "function": "test_get_object_multipart", "status": "PASS"}
```

* The first part illustrates the compatibility tests against an S3 test endpoint, provided by the MinIO project, and using the libs3 library, i.e., using the s3 executable found within the libs3 library.
* The second part shows that after switching to S3Embedded the same tests are accomplished against the local tmpfs filesystem */dev/shm*.
As to be expected, we can see that the duration of each test is obviously smaller. 

## References
Please refer to the published paper entitled [Analyzing the Performance of the S3 Object Storage API for HPC Workloads](https://doi.org/10.3390/app11188540) to gain more insights into the context and the incentives behind this work.
