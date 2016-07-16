#!/bin/bash

SplitCorpus() {
  #arg1: path of the single file corpus

  echo "Splitting Corpus"

  cd corpus
  split -b 100M $1
  python ../src/quantizing.py ../corpus #path of the single file corpus
}


MakePartialIndexes() {
  #arg1: dir containing the split corpus
  #arg2: dir to store partial indexes
  
  echo "Creating Partial Indexes"

  # number of process
  max_parallel_count=4;
  count=0
  index_counter=0

  for i in `ls $1`
  do
  	count=$((count+1))
  	index_counter=$((index_counter+1))

  	echo $i
	  ./indexer $1/$i > $2/"pi_"$i &

  	if [ $count -eq $max_parallel_count ]
  	then
	  	wait
  		count=0
  		echo $index_counter
  	fi
  done

  if [$count -ne 0 ]
  then
  	wait
  	count=0
  	echo $index_counter
  fi
}


case "$1" in
  1)
    SplitCorpus $2
    ;;
  2)
    MakePartialIndexes $2 $3
    ;;
  *)
    echo "Argument value should be either 1 or 2."
    ;;
esac
