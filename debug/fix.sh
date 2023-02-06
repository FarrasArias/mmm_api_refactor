FILEPATH=/users/jeff/CODE/MMM_API/debug/jun_7/piece.json
gsed -i 's/isDrum" : 0/isDrum" : false/' $FILEPATH
gsed -i 's/isDrum" : 1/isDrum" : true/' $FILEPATH
gsed -i 's/resample" : 0/resample" : false/' $FILEPATH
gsed -i 's/resample" : 1/resample" : true/' $FILEPATH
gsed -i 's/ignore" : 0/ignore" : false/' $FILEPATH
gsed -i 's/ignore" : 1/ignore" : true/' $FILEPATH
gsed -i 's/selectedBars" : \[ 0, 0, 0, 0 \]/selectedBars" : [ false, false, false, false ]/' $FILEPATH
gsed -i 's/selectedBars" : \[ 1, 1, 1, 1 \]/selectedBars" : [ true, true, true, true ]/' $FILEPATH
