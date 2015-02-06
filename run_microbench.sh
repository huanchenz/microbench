#!bin/bash

MERGE_THRESHOLD=1000
MERGE_RATIO=10

OUTPUT_MICROBENCH="outputs/output_microbench.csv"


./stdmap_a_int > $OUTPUT_MICROBENCH
./stxbtree_a_int >> $OUTPUT_MICROBENCH
./hybrid_a_int $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH
./stdmap_c_int >> $OUTPUT_MICROBENCH
./stxbtree_c_int >> $OUTPUT_MICROBENCH
./hybrid_c_int $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH
./stdmap_e_int >> $OUTPUT_MICROBENCH
./stxbtree_e_int >> $OUTPUT_MICROBENCH
./hybrid_e_int $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH


./stdmap_a_str >> $OUTPUT_MICROBENCH
./stxbtree_a_str>> $OUTPUT_MICROBENCH
./hybrid_a_str $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH
./stdmap_c_str >> $OUTPUT_MICROBENCH
./stxbtree_c_str >> $OUTPUT_MICROBENCH
./hybrid_c_str $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH
./stdmap_e_str >> $OUTPUT_MICROBENCH
./stxbtree_e_str >> $OUTPUT_MICROBENCH
./hybrid_e_str $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH

./stdmap_a_url >> $OUTPUT_MICROBENCH
./stxbtree_a_url>> $OUTPUT_MICROBENCH
./hybrid_a_url $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH
./stdmap_c_url >> $OUTPUT_MICROBENCH
./stxbtree_c_url >> $OUTPUT_MICROBENCH
./hybrid_c_url $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH
./stdmap_e_url >> $OUTPUT_MICROBENCH
./stxbtree_e_url >> $OUTPUT_MICROBENCH
./hybrid_e_url $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH

