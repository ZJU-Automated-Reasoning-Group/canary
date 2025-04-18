import argparse
import csv

# Function to read CSV data
def read_csv(file_path):
    data = []
    with open(file_path, 'r') as file:
        reader = csv.DictReader(file)
        for row in reader:
            data.append(row)
    return data

# Function to write filtered data back to CSV
def write_csv(file_path, data, fieldnames):
    file_path = file_path + "_new.csv"
    with open(file_path, 'w', newline='') as file:
        writer = csv.DictWriter(file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(data)

# Function to filter out rows based on the conditions for lower and upper bounds
def filter_boundaries(data):
    filtered_data = []
    
    for entry in data:
        if not (entry['Bad Solve'] == '1'):
            filtered_data.append(entry)
    
    return filtered_data

# Main function
def main():
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('-F', '--file', type=str, required=True, help="Source file")
    args=parser.parse_args()

    # Path to the CSV file
    file_path = args.file  # Update this path
    
    # Read data from CSV
    data = read_csv(file_path)
    
    # Filter out rows with the specified bounds condition
    filtered_data = filter_boundaries(data)
    
    # If data has been filtered, write it back to the CSV file
    if len(filtered_data) < len(data):
        print(f"Filtered out {len(data) - len(filtered_data)} rows.")
        fieldnames = data[0].keys()
        write_csv(file_path, filtered_data, fieldnames)
    else:
        print("No rows were filtered.")
    
if __name__ == "__main__":
    main()
