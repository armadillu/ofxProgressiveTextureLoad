#!/usr/bin/env bash
set -e

echo "Executing ci/vs/install.sh"

export OF_ROOT="$APPVEYOR_BUILD_FOLDER/openFrameworks"
export OF_ADDONS=$OF_ROOT/addons

pwd;
echo "OF_ROOT: $OF_ROOT"

ADDONS="armadillu/ofxHistoryPlot armadillu/ofxTimeMeasurements"


cd $OF_ADDONS

for ADDON in $ADDONS
do
  echo "Cloning addon '$ADDON' to " `pwd`
  git clone --depth=1 --branch=$OF_BRANCH https://github.com/$ADDON.git
done

cd -