#!bin/bash

MERGE_THRESHOLD=1000
MERGE_RATIO=10

MERGE_THRESHOLD_MT=1000001
MERGE_RATIO_MT=0

MERGE_THRESHOLD_SMT=1000000
MERGE_RATIO_SMT=0

OUTPUT_MICROBENCH="outputs/output_microbench.csv"
OUTPUT_MT_VS_SMT="outputs/output_mt_vs_smt.csv"

OUTPUT_MERGECOST_INT_TIMELINE="outputs/output_mergecost_int_timeline.csv"
OUTPUT_MERGECOST_STR_TIMELINE="outputs/output_mergecost_str_timeline.csv"
OUTPUT_MERGECOST_URL_TIMELINE="outputs/output_mergecost_url_timeline.csv"

OUTPUT_MERGETIME_INT="outputs/output_mergetime_int.csv"
OUTPUT_MERGETIME_STR="outputs/output_mergetime_str.csv"
OUTPUT_MERGETIME_URL="outputs/output_mergetime_url.csv"

OUTPUT_MERTE_CONST="outputs/output_merge_const.csv"
OUTPUT_MERTE_RATIO="outputs/output_merge_ratio.csv"

#OUTPUT_URL="outputs/output_url.csv"


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


./hybrid_c_int $MERGE_THRESHOLD_MT $MERGE_RATIO_MT > $OUTPUT_MT_VS_SMT
./hybrid_c_int $MERGE_THRESHOLD_SMT $MERGE_RATIO_SMT >> $OUTPUT_MT_VS_SMT
./hybrid_c_str $MERGE_THRESHOLD_MT $MERGE_RATIO_MT >> $OUTPUT_MT_VS_SMT
./hybrid_c_str $MERGE_THRESHOLD_SMT $MERGE_RATIO_SMT >> $OUTPUT_MT_VS_SMT
./hybrid_c_url $MERGE_THRESHOLD_MT $MERGE_RATIO_MT >> $OUTPUT_MT_VS_SMT
./hybrid_c_url $MERGE_THRESHOLD_SMT $MERGE_RATIO_SMT >> $OUTPUT_MT_VS_SMT

#./stdmap_c_url > $OUTPUT_URL
#./stxbtree_c_url >> $OUTPUT_URL
#./hybrid_c_url $MERGE_THRESHOLD_MT $MERGE_RATIO_MT >> $OUTPUT_URL
#./hybrid_c_url $MERGE_THRESHOLD_SMT $MERGE_RATIO_SMT >> $OUTPUT_URL


./merge_cost_int $MERGE_THRESHOLD $MERGE_RATIO > $OUTPUT_MERGECOST_INT_TIMELINE
./merge_cost_str $MERGE_THRESHOLD $MERGE_RATIO > $OUTPUT_MERGECOST_STR_TIMELINE
./merge_cost_url $MERGE_THRESHOLD $MERGE_RATIO > $OUTPUT_MERGECOST_URL_TIMELINE

./merge_time_int $MERGE_THRESHOLD $MERGE_RATIO > $OUTPUT_MERGETIME_INT
./merge_time_str $MERGE_THRESHOLD $MERGE_RATIO > $OUTPUT_MERGETIME_STR
./merge_time_url $MERGE_THRESHOLD $MERGE_RATIO > $OUTPUT_MERGETIME_URL

