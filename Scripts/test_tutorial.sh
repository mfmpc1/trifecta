#!/usr/bin/env bash

while getopts XYC opt; do
    case $opt in
	X) compile_opts=-X
	   dabit=1
	   ;;
	Y) dabit=2
	   ;;
    esac
done

shift $[OPTIND-1]

for i in 0 1; do
    seq 0 3 > Player-Data/Input-P$i-0
done

# clean state
rm Player-Data/*Params*
rm Player-Data/*Secrets*

function test_vm
{
    ulimit -c unlimited
    vm=$1
    shift
    if ! Scripts/$vm.sh tutorial $* | grep 'weighted average: 2.333'; then
	for i in 0 1 2; do
	    echo == Party $i
	    cat logs/tutorial-$i
	done
	exit 1
    fi
}

# big buckets for smallest batches
run_opts="$run_opts -B 5"

for dabit in ${dabit:-0 1 2}; do
    if [[ $dabit = 1 ]]; then
	compile_opts="$compile_opts -X"
    elif [[ $dabit = 2 ]]; then
	compile_opts="$compile_opts -Y"
    fi

    ./compile.py -R 64 $compile_opts tutorial

    for i in ring semi2k brain mal-rep-ring ps-rep-ring spdz2k; do
	test_vm $i $run_opts
    done

    ./compile.py  $compile_opts tutorial

    for i in rep-field shamir mal-rep-field ps-rep-field mal-shamir hemi semi \
		       soho cowgear mascot; do
	test_vm $i $run_opts
    done

    test_vm chaigear $run_opts -l 3 -c 2
done

./compile.py tutorial

test_vm cowgear -T
test_vm chaigear -T -l 3 -c 2

./compile.py -B 16  $compile_opts tutorial

for i in replicated mal-rep-bin semi-bin ccd mal-ccd yao tinier rep-bmr mal-rep-bmr shamir-bmr mal-shamir-bmr; do
    test_vm $i
done
