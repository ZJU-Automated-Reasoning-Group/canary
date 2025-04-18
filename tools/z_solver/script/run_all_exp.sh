timeout=60

#python3 exp.py -P ../bin/bestInv -D ../data/novesafe -M test -O ../results/octtest.csv -A octagon -T ${timeout}
#python3 exp.py -P ../bin/bestInv -D ../data/novesafe -M bi -O ../results/octbi.csv -A octagon -T ${timeout}
#python3 exp.py -P ../bin/bestInv -D ../data/novesafe -M decr -O ../results/octdecr.csv -A octagon -T ${timeout}
#python3 exp.py -P ../bin/bestInv -D ../data/novesafe -M bi -O ../results/bitbi.csv -A bitwise -T ${timeout}
#python3 exp.py -P ../bin/bestInv -D ../data/novesafe -M test -O ../results/bittest.csv -A bitwise -T ${timeout}

python3 exp.py -P ../bin/kInductor -D ../data/nove -M none -O ../results/indraw.csv -A none -T -1
python3 exp.py -P ../bin/kInductor -D ../data/nove -M test -O ../results/indtest.csv -A interval -T 1