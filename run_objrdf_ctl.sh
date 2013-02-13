#!/bin/bash
# set the context for the application, the database is supposed to be in db/
# the directory should contain the file `schema.so'
LD_LIBRARY_PATH=$1:$LD_LIBRARY_PATH  OBJRDF_DB=$1 ./objrdf_ctl `basename $0`

