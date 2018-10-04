#!/bin/env bash

TYPE=$1
NAME=$2

# camel case
NAME_FILE=`echo "$NAME" | sed -r 's/(-)(\w)/\U\2/g'`
NAME_CLASS=`echo "$NAME" | sed -r 's/(^|-)(\w)/\U\2/g'`
NAME_INCGUARD=`echo "$NAME" | sed -e 's/\(.*\)/\U\1/' | sed -e 's/-/_/g'`_HPP

INC_DIR=./source/inc/$TYPE/
SRC_DIR=./source/src/$TYPE/
INC_FILEPATH=$INC_DIR$NAME_FILE.hpp
SRC_FILEPATH=$SRC_DIR$NAME_FILE.cpp

echo $NAME_INCGUARD
echo $INC_FILEPATH
echo $SRC_FILEPATH

if [ ! -e $INC_DIR ]; then
    mkdir $INC_DIR
fi
if [ ! -e $SRC_DIR ]; then
    mkdir $SRC_DIR
fi
if [ -e $INC_FILEPATH ] || [ -e $SRC_FILEPATH ]; then
    echo "ERROR: File(s) arleady exist -->"
    echo "  $INC_FILEPATH"
    echo "  $SRC_FILEPATH"
    exit 1
fi

echo -e "#ifndef $NAME_INCGUARD" > $INC_FILEPATH
echo -e "#define $NAME_INCGUARD" >> $INC_FILEPATH
echo -e "\nclass $NAME_CLASS\n{" >> $INC_FILEPATH
echo -e " public:\n    $NAME_CLASS();" >> $INC_FILEPATH
echo -e "    ~$NAME_CLASS();" >> $INC_FILEPATH
echo -e " private:" >> $INC_FILEPATH
echo -e "};\n" >> $INC_FILEPATH
echo -e "#endif // $NAME_INCGUARD" >> $INC_FILEPATH


echo -e "#include \"$NAME_FILE.hpp\"" > $SRC_FILEPATH
echo -e "\n$NAME_CLASS::$NAME_CLASS()\n{\n\n}\n" >> $SRC_FILEPATH
