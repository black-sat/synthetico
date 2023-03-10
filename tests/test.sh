#!/bin/bash

trap 'exit 130' INT

die() {
  echo "$@" >&2
}

if [ -z "$1" ]; then
  die "Please provide a tests file"
fi

tests="$(pwd)/$1"

cd "$(git rev-parse --show-toplevel)/build"

echo "Instance novel novel-out classic classic-out"
i=1
while IFS= read -r in; do 
  echo -n "$i "
  for algo in novel classic; do 
    time=$( \
      (\time -f %e \
        timeout 10 \
        bash -c "./synth $algo $in > ./out-$i.txt 2> /dev/null" \
      ) 2>&1 \
    )
    output=$(cat ./out-$i.txt)
    if echo $time | grep -q 'Command'; then 
      echo -n "TIMEOUT TIMEOUT "
    else
      echo -n "$time $output "
    fi
  done
  echo
  i=$((i + 1))
done < "$1"