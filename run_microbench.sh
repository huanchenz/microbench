#!bin/bash

MERGE_THRESHOLD=10000
MERGE_RATIO=10

MERGE_THRESHOLD_MT=100000000
MERGE_RATIO_MT=0

MERGE_THRESHOLD_SMT=50000000
MERGE_RATIO_SMT=0


OUTPUT_MICROBENCH="outputs/output_microbench.csv"

echo "1/36"
./stdmap_a_int > $OUTPUT_MICROBENCH
echo "2/36"
./stxbtree_a_int >> $OUTPUT_MICROBENCH
echo "3/36"
./hybrid_a_int $MERGE_THRESHOLD_MT $MERGE_RATIO_MT >> $OUTPUT_MICROBENCH
echo "4/36"
./hybrid_a_int $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH
echo "5/36"
./stdmap_c_int >> $OUTPUT_MICROBENCH
echo "6/36"
./stxbtree_c_int >> $OUTPUT_MICROBENCH
echo "7/36"
./hybrid_c_int $MERGE_THRESHOLD_MT $MERGE_RATIO_MT >> $OUTPUT_MICROBENCH
echo "8/36"
./hybrid_c_int $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH
echo "9/36"
./stdmap_e_int >> $OUTPUT_MICROBENCH
echo "10/36"
./stxbtree_e_int >> $OUTPUT_MICROBENCH
echo "11/36"
./hybrid_e_int $MERGE_THRESHOLD_MT $MERGE_RATIO_MT >> $OUTPUT_MICROBENCH
echo "12/36"
./hybrid_e_int $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH

echo "13/36"
./stdmap_a_str >> $OUTPUT_MICROBENCH
echo "14/36"
./stxbtree_a_str>> $OUTPUT_MICROBENCH
echo "15/36"
./hybrid_a_str $MERGE_THRESHOLD_MT $MERGE_RATIO_MT >> $OUTPUT_MICROBENCH
echo "16/36"
./hybrid_a_str $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH
echo "17/36"
./stdmap_c_str >> $OUTPUT_MICROBENCH
echo "18/36"
./stxbtree_c_str >> $OUTPUT_MICROBENCH
echo "19/36"
./hybrid_c_str $MERGE_THRESHOLD_MT $MERGE_RATIO_MT >> $OUTPUT_MICROBENCH
echo "20/36"
./hybrid_c_str $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH
echo "21/36"
./stdmap_e_str >> $OUTPUT_MICROBENCH
echo "22/36"
./stxbtree_e_str >> $OUTPUT_MICROBENCH
echo "23/36"
./hybrid_e_str $MERGE_THRESHOLD_MT $MERGE_RATIO_MT >> $OUTPUT_MICROBENCH
echo "24/36"
./hybrid_e_str $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH

#echo "25/36"
#./stdmap_a_url >> $OUTPUT_MICROBENCH
#echo "26/36"
#./stxbtree_a_url>> $OUTPUT_MICROBENCH
#echo "27/36"
#./hybrid_a_url $MERGE_THRESHOLD_MT $MERGE_RATIO_MT >> $OUTPUT_MICROBENCH
#echo "28/36"
#./hybrid_a_url $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH
#echo "29/36"
#./stdmap_c_url >> $OUTPUT_MICROBENCH
#echo "30/36"
#./stxbtree_c_url >> $OUTPUT_MICROBENCH
#echo "31/36"
#./hybrid_c_url $MERGE_THRESHOLD_MT $MERGE_RATIO_MT >> $OUTPUT_MICROBENCH
#echo "32/36"
#./hybrid_c_url $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH
#echo "33/36"
#./stdmap_e_url >> $OUTPUT_MICROBENCH
#echo "34/36"
#./stxbtree_e_url >> $OUTPUT_MICROBENCH
#echo "35/36"
#./hybrid_e_url $MERGE_THRESHOLD_MT $MERGE_RATIO_MT >> $OUTPUT_MICROBENCH
#echo "36/36"
#./hybrid_e_url $MERGE_THRESHOLD $MERGE_RATIO >> $OUTPUT_MICROBENCH

