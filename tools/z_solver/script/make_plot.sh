python3 extract_safe.py -F /home/zarin/z_solver/results/cmpr.csv
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/solved_time_safe.pdf --plot 3
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_instances_safe.pdf --plot 2

python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_ci_omt_bs.pdf --m1 omtfix --m2 binfix --plot 0
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_ci_omt_bi.pdf --m1 omtfix --m2 bi --plot 0
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_ci_bs_bi.pdf --m1 binfix --m2 bi --plot 0
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_cp_bi_decr.pdf --m1 bi --m2 decr --plot 0
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_cp_bi_test.pdf --m1 bi --m2 test --plot 0
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_ef_decr_test.pdf --m1 decr --m2 test --plot 0
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_cp_bi_decr.pdf --m1 bi --m2 decr --plot 0
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_cp_bi_test.pdf --m1 bi --m2 test --plot 0
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_ef_decr_test.pdf --m1 decr --m2 test --plot 0

python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_bit_bi.pdf --m1 bi --plot 4
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_bit_decr.pdf --m1 decr --plot 4
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_bit_test.pdf --m1 test --plot 4

python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_differbit_bi.pdf --m1 bi --plot 5
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_differbit_decr.pdf --m1 decr --plot 5
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_differbit_test.pdf --m1 test --plot 5

python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_differbit_bi.pdf --m1 bi --plot 5
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_differbit_decr.pdf --m1 decr --plot 5
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/time_differbit_test.pdf --m1 test --plot 5

python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/call_bit_bi.pdf --m1 bi --plot 6
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/call_bit_decr.pdf --m1 decr --plot 6
python3 analysis.py -F /home/zarin/z_solver/results/cmpr.csv_new.csv --save --path /home/zarin/z_solver/results/call_bit_test.pdf --m1 test --plot 6
