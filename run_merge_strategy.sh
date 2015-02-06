#!bin/bash

MERGE_THRESHOLD=1000
MERGE_RATIO=10

MERGE_THRESHOLD_MT=1000001
MERGE_RATIO_MT=0

MERGE_THRESHOLD_SMT=1000000
MERGE_RATIO_SMT=0

OUTPUT_MERGE_CONST="outputs/output_merge_const.csv"
OUTPUT_MERGE_RATIO="outputs/output_merge_ratio.csv"

#OUTPUT_URL="outputs/output_url.csv"


merge_ratio=0
echo -n "" > $OUTPUT_MERGE_CONST
for merge_threshold in 100 200 500 1000 2000 5000 10000 20000 50000 100000; do
    ./hybrid_c_int_merge $merge_threshold $merge_ratio >> $OUTPUT_MERGE_CONST
done

merge_threshold=1000
echo -n "" > $OUTPUT_MERGE_RATIO
for merge_ratio in 1 2 5 10 20 50 100 200 500; do
    ./hybrid_c_int_merge $merge_threshold $merge_ratio >> $OUTPUT_MERGE_RATIO
done

merge_ratio=0
for merge_threshold in 100 200 500 1000 2000 5000 10000 20000 50000 100000; do
    ./hybrid_c_str_merge $merge_threshold $merge_ratio >> $OUTPUT_MERGE_CONST
done

merge_threshold=1000
for merge_ratio in 1 2 5 10 20 50 100 200 500; do
    ./hybrid_c_str_merge $merge_threshold $merge_ratio >> $OUTPUT_MERGE_RATIO
done

merge_ratio=0
for merge_threshold in 100 200 500 1000 2000 5000 10000 20000 50000 100000; do
    ./hybrid_c_url_merge $merge_threshold $merge_ratio >> $OUTPUT_MERGE_CONST
done

merge_threshold=1000
for merge_ratio in 1 2 5 10 20 50 100 200 500; do
    ./hybrid_c_url_merge $merge_threshold $merge_ratio >> $OUTPUT_MERGE_RATIO
done

