#!/bin/bash
# Run qdtrajectory for each case in cases.txt and compare with the expected output

DATA=/usr/share/smartmet/test/data/ecpainepinta/200809090714_ecmwf_skandinavia_painepinta240h.sqd

if [ -x ../qdtrajectory ]; then
    PROGRAM=../qdtrajectory
    TEMPLATES=../tmpl
    export LD_LIBRARY_PATH=..${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
else
    PROGRAM=qdtrajectory
    TEMPLATES=/usr/share/smartmet/trajectory
fi

mkdir -p failures
rm -f failures/*

ok=0
failed=0
while IFS='|' read -r name args; do
    name=$(echo $name)
    [[ -z "$name" || "$name" == \#* ]] && continue
    result=failures/$name
    printf "%-40s" "$name"
    if ! $PROGRAM -q $DATA --templatedir $TEMPLATES $args > $result 2> $result.stderr; then
        echo "FAIL (exit code $?)"
        cat $result.stderr
        failed=$((failed+1))
    elif [ ! -f output/$name ]; then
        echo "FAIL (output/$name missing)"
        failed=$((failed+1))
    elif cmp -s output/$name $result; then
        echo "OK"
        rm -f $result $result.stderr
        ok=$((ok+1))
    else
        echo "FAIL"
        diff output/$name $result | head -10
        failed=$((failed+1))
    fi
done < cases.txt

echo
echo "$ok passed, $failed failed"
[ $failed -eq 0 ]
