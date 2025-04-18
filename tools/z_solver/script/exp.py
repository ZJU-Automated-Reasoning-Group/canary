import os
import re
import csv
import time
import psutil
import argparse
import subprocess

def dir_run(program, dir, method, output, domain, timeout):
    timeout_csv = os.path.join(os.path.dirname(os.path.abspath(__file__)), "timeout.csv")

    for root, dirs, files in os.walk(dir):
        for filename in files:
            if filename.endswith(".smt2"):
                file_path = os.path.join(root, filename)
                
                print(f"dealing with {file_path}...")

                command = [
                    program,
                    "-F", file_path,
                    "-M", method,
                    "-O", output,
                    "-D", domain,
                    "-T", str(timeout)
                ]

                try:
                    if timeout == -1:
                        subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                    else:
                        try:
                            subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
                        except subprocess.TimeoutExpired:
                            with open(timeout_csv, "a", newline = '') as csvfile:
                                writer = csv.writer(csvfile)
                                writer.writerow([file_path, method, timeout])
                except subprocess.CalledProcessError:
                    continue

def list_run(program, list, output, domain, timeout):
    timeout_csv = os.path.join(os.path.dirname(os.path.abspath(__file__)), "timeout.csv")

    data = []
    with open(list, 'r') as file:
        reader = csv.DictReader(file)
        for row in reader:
            data.append(row)

    for entry in data:
        file_path = entry['File']
                
        print(f"dealing with {file_path}...")

        command = [program,
            "-F", file_path,
            "-M", entry['Method'],
            "-O", output,
            "-D", domain,
            "-T", str(timeout)]

        try:
            if timeout == -1:
                subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            else:
                try:
                    subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
                except subprocess.TimeoutExpired:
                    with open(timeout_csv, "a", newline = '') as csvfile:
                        writer = csv.writer(csvfile)
                        writer.writerow([file_path, method, timeout])
        except subprocess.CalledProcessError:
            continue

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('-D', '--dir', type=str, help="Source directory")
    parser.add_argument('-L', '--list', type=str, help="Path of Timeout.csv (for Incremental)")
    parser.add_argument('-P', '--program', type=str, required=True, help="Path of z_solver")
    parser.add_argument('-M', '--method', type=str, help="Algorithm")
    parser.add_argument('-O', '--output', type=str, required=True, help="Output file")
    parser.add_argument('-T', '--timeout', type=int, default=-1, help="Timeout of solving")
    parser.add_argument('-A', '--domain', type=str, required=True, help="Abstract domain")
    args=parser.parse_args()

    script_dir = os.path.dirname(os.path.abspath(__file__))
    args.program = os.path.join(script_dir, args.program)

    if args.dir != None:
        args.dir = os.path.join(script_dir, args.dir)
        dir_run(args.program, args.dir, args.method, args.output, args.domain, args.timeout)
    elif args.list != None:
        args.list = os.path.join(script_dir, args.list)
        list_run(args.program, args.list, args.output, args.domain, args.timeout)