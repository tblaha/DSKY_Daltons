#!/bin/sh
set -e

if [ ! -f .fuse ]
then
    echo "Refusing to run; not in git root directory"
    exit 1
fi

# remove old directories
rm -rf DSKY_Daltons
mkdir DSKY_Daltons
cd DSKY_Daltons

# pull the latest stuff
echo "DSKY_Daltons_Project\n\n\n" | dueca-project checkout
echo "DSKY_Daltons_Dynamics\n\n\n" | dueca-project checkout
echo "DSKY_Daltons_Controller\n\n\n" | dueca-project checkout
echo "DSKY_Daltons_HUD\n\n\n" | dueca-project checkout



# commit
cd ..

git add .
git commit -m "Automatic mirror from dueca.tudelft.nl at $(date --rfc-3339=seconds)"


