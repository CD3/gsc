#! /bin/bash

set -e

if [[ -z $1 ]]
then
  echo "usage: $0 <NAME>"
  exit 1
fi

name=$1
shift


source=$(dirname $(dirname $(dirname $0)))
dest=$name

rsync --recursive --links \
      --filter="- /testing/CMakeTests/" \
      --filter="- /build/" \
      --filter="- /externals/*/" \
      --filter="- /externals/*" \
      --filter="- .git/" \
      $source/ $dest
cd $dest

ag -l --ignore-dir scripts TemplateProject | xargs sed -i "s/TemplateProject/$name/g"

for file in $(find ./ -iname '*TemplateProject*' | ag -v "\.\/build\/")
do
  nfile=$(echo $file | sed "s/TemplateProject/$name/g")
  mv $file $nfile
done

year=$(date +%Y)
sed -i "s/<YEAR>/$year/" LICENSE.md

git init
git add .
git commit -m "initial import"
