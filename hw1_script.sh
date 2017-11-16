rm -rf $HW1DIR
mkdir $HW1DIR
chmod 755 $HW1DIR
cd $HW1DIR
cp ${1} $HW1TF
chmod 754 $HW1TF
./hw1_subs ABC 123
