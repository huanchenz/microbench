#!bin/bash

OUTPUT_MERGE_CONST="outputs/output_merge_const.csv"
OUTPUT_MERGE_RATIO="outputs/output_merge_ratio.csv"

#OUTPUT_URL="outputs/output_url.csv"


merge_ratio=0
echo -n "" > $OUTPUT_MERGE_CONST
#for merge_threshold in 100 200 500 1000 2000 5000 10000 20000 50000 100000; do
for merge_threshold in 10000 20000 40000 80000 100000; do
    echo "const"
    echo $merge_threshold
    ./hybrid_c_int_merge $merge_threshold $merge_ratio >> $OUTPUT_MERGE_CONST
done

merge_threshold=10000
echo -n "" > $OUTPUT_MERGE_RATIO
#for merge_ratio in 1 2 5 10 20 50 100 200 500; do
for merge_ratio in 1 2 4 6 8 10 20 40 60 80 100 200; do
    echo "ratio"
    echo $merge_ratio
    ./hybrid_c_int_merge $merge_threshold $merge_ratio >> $OUTPUT_MERGE_RATIO
done

#merge_ratio=0
#for merge_threshold in 100 200 500 1000 2000 5000 10000 20000 50000 100000; do
#for merge_threshold in 100 200 400 500 800 1000 2000 4000 5000 8000 10000 20000; do
#    ./hybrid_c_str_merge $merge_threshold $merge_ratio >> $OUTPUT_MERGE_CONST
#done

#merge_threshold=1000
#for merge_ratio in 1 2 5 10 20 50 100 200 500; do
#for merge_ratio in 1 2 4 8 10 20 40 60 80 100 200; do
#    ./hybrid_c_str_merge $merge_threshold $merge_ratio >> $OUTPUT_MERGE_RATIO
#done

#merge_ratio=0
#for merge_threshold in 100 200 500 1000 2000 5000 10000 20000 50000 100000; do
#for merge_threshold in 100 200 400 500 800 1000 2000 4000 5000 8000 10000 20000; do
#    ./hybrid_c_url_merge $merge_threshold $merge_ratio >> $OUTPUT_MERGE_CONST
#done

#merge_threshold=1000
#for merge_ratio in 1 2 5 10 20 50 100 200 500; do
#for merge_ratio in 1 2 4 8 10 20 40 60 80 100 200; do
#    ./hybrid_c_url_merge $merge_threshold $merge_ratio >> $OUTPUT_MERGE_RATIO
#done

