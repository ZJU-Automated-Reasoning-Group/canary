import os
import re
import time
import psutil
import argparse
import subprocess

def dir_run(program, dir, output, timeout):
    if not os.path.exists(dir):
        print(f"Directory '{dir}' does not exist.")
        return

    for root, dirs, files in os.walk(dir):
        for filename in files:
            if filename.endswith(".smt2"):
                file_path = os.path.join(root, filename)
                
                print(f"dealing with {file_path}...")

                for method in ['decr', 'test']:
                    command = [
                        program,
                        "-F", file_path,
                        "-M", method,
                        "-O", output
                    ]

                    try:
                        if timeout == -1:
                            subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                        else:
                            try:
                                subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
                            except subprocess.TimeoutExpired:
                                print(f"Critical: {file_path} expired!")
                    except subprocess.CalledProcessError:
                        continue

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('-D', '--dir', type=str, required=True, help="Source directory")
    parser.add_argument('-P', '--program', type=str, required=True, help="Path of z_solver")
    parser.add_argument('-O', '--output', type=str, required=True, help="Output file")
    parser.add_argument('-T', '--timeout', type=int, default=-1, help="Timeout of solving")
    args=parser.parse_args()

    script_path = os.path.abspath(__file__)
    script_dir = os.path.dirname(script_path)
    args.dir = os.path.join(script_dir, args.dir)
    args.program = os.path.join(script_dir, args.program)

    dir_run(args.program, args.dir, args.output, args.timeout)