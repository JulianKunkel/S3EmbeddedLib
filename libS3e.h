#ifndef LIBS3E_H
#define LIBS3E_H

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>


static inline unsigned hash_func(char const * name){
  unsigned hash=0;
  for(char const * c = name; *c != '\0'; c++){
    hash = (hash << 3) + *c;
  }
  return hash % (10001); // max 10001 directories to be used
}


#ifdef S3_EXTRA_BUCKET
#define SET_BUCKET_HASH_DIR(buf, hostname, name) sprintf(buf, "%s/%u", hostname ? hostname : opt.dirname, hash_func(name));
#define SET_BUCKET_NAME(buf, hostname, name) sprintf(buf, "%s/%u/%s", hostname ? hostname : opt.dirname, hash_func(name), name);
#define SET_OBJECT_NAME(buf, hostname, name, key) sprintf(buf, "%s/%u/%s/%s", hostname ? hostname : opt.dirname, hash_func(name), name, key);
#define SET_NAME(buf, name) sprintf(buf, "%s/%u", opt.dirname, hash_func(name));

#else
#define SET_BUCKET_HASH_DIR(buf, hostname, name) sprintf(buf, "%s/%s", hostname ? hostname : opt.dirname, name);
#define SET_BUCKET_NAME(buf, hostname, name) sprintf(buf, "%s/%s", hostname ? hostname : opt.dirname, name);
#define SET_OBJECT_NAME(buf, hostname, name, key) sprintf(buf, "%s/%s/%s", hostname ? hostname : opt.dirname, name, key);
#define SET_NAME(buf, name) sprintf(buf, "%s/%s", opt.dirname, name);

#endif

#endif
