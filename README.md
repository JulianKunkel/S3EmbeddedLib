# S3EmbeddedLib
This repository contains a series of fake implementations of LibS3 that can be used as replacement of LibS3, i.e., it can be linked together with applications.
The purpose is to allow convenient testing of a LibS3 API usage and the evaluation of performance implications.

It requires the original header of the libS3.h.
During the linking process any program can be linked with this library.

The S3libs here depend on a global accessible shared file system to store the data.

The following libraries are provided:
  * libS3e.so: This is an embedded library wrapper that converts libS3 calls to POSIX calls.
  * libS3r.so: This library converts the libS3 calls via a binary conversion to TCP calls to a local libS3-gw application that then executes these POSIX calls.

# Options
At runtime specify the directory path as hostname in S3_initialize.
