# S3EmbeddedLib
This is a fake Implementation of LibS3 such that it can be linked together with applications to measure the costs for external communication.

It depends on a global accessible shared file system to store the data.

It requires the original header of the libS3.h but during the linking process a program can be linked with this library.

# Options
At runtime specify the directory path as hostname in S3_initialize.
