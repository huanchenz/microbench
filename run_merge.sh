#!bin/bash

MERGE_THRESHOLD=1000
MERGE_RATIO=10

MERGE_RATIO_ZERO=0

OUTPUT_MERGECOST_INT_TIMELINE="outputs/output_mergecost_int_timeline.csv"
OUTPUT_MERGECOST_STR_TIMELINE="outputs/output_mergecost_str_timeline.csv"
OUTPUT_MERGECOST_URL_TIMELINE="outputs/output_mergecost_url_timeline.csv"

OUTPUT_MERGECOST_INT_TIMELINE_CONST="outputs/output_mergecost_int_timeline_const.csv"
OUTPUT_MERGECOST_STR_TIMELINE_CONST="outputs/output_mergecost_str_timeline_const.csv"
OUTPUT_MERGECOST_URL_TIMELINE_CONST="outputs/output_mergecost_url_timeline_const.csv"

OUTPUT_MERGETIME_INT="outputs/output_mergetime_int.csv"
OUTPUT_MERGETIME_STR="outputs/output_mergetime_str.csv"
OUTPUT_MERGETIME_URL="outputs/output_mergetime_url.csv"


./merge_cost_int $MERGE_THRESHOLD $MERGE_RATIO > $OUTPUT_MERGECOST_INT_TIMELINE
./merge_cost_str $MERGE_THRESHOLD $MERGE_RATIO > $OUTPUT_MERGECOST_STR_TIMELINE
./merge_cost_url $MERGE_THRESHOLD $MERGE_RATIO > $OUTPUT_MERGECOST_URL_TIMELINE

./merge_cost_int $MERGE_THRESHOLD $MERGE_RATIO_ZERO > $OUTPUT_MERGECOST_INT_TIMELINE_CONST
./merge_cost_str $MERGE_THRESHOLD $MERGE_RATIO_ZERO > $OUTPUT_MERGECOST_STR_TIMELINE_CONST
./merge_cost_url $MERGE_THRESHOLD $MERGE_RATIO_ZERO > $OUTPUT_MERGECOST_URL_TIMELINE_CONST

#./merge_time_int $MERGE_THRESHOLD $MERGE_RATIO > $OUTPUT_MERGETIME_INT
#./merge_time_str $MERGE_THRESHOLD $MERGE_RATIO > $OUTPUT_MERGETIME_STR
#./merge_time_url $MERGE_THRESHOLD $MERGE_RATIO > $OUTPUT_MERGETIME_URL

