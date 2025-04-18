import argparse
import csv
import re

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

# Function to clean and check if the remaining string is all 0s
def is_all_zeros(value):
    # Remove unwanted characters
    cleaned_value = value.replace('[', '').replace(']', '').replace('#x', '').replace(',', '').strip()
    
    # Check if the cleaned string is all zeros or all f's
    #return cleaned_value == '0' * len(cleaned_value)
    return cleaned_value.count('0') >= len(cleaned_value) / 2

# Function to clean and check if the remaining string is all fs
def is_all_fs(value):
    # Remove unwanted characters
    cleaned_value = value.replace('[', '').replace(']', '').replace('#x', '').replace(',', '').strip()
    
    # Check if the cleaned string is all zeros or all f's
    #return cleaned_value == 'f' * len(cleaned_value)
    return cleaned_value.count('f') > len(cleaned_value) / 2

def detect_corresponding_pairs(s1, s2):
    """
    Detects if there is a '#x00000000' in the first string that corresponds
    to '#xffffffff' in the second string at the same index.

    Args:
        s1 (str): The first string containing the '#x00000000' substrings.
        s2 (str): The second string containing the '#xffffffff' substrings.

    Returns:
        bool: True if a '#x00000000' corresponds to a '#xffffffff' at the same index in both strings, False otherwise.
    """
    # Remove the leading "[#" and trailing "]" from the strings and split by commas
    s1 = s1.strip('[]').split(',')
    s2 = s2.strip('[]').split(',')
    
    # Check that both lists are of the same length
    if len(s1) != len(s2):
        return False


    num = 0
    # Compare corresponding elements
    for i in range(len(s1)):
        if re.match(r'#x0+0$', s1[i]) and re.match(r'#x[f]+$', s2[i]):
            num = num + 1  # A match is found where '0...' corresponds to 'f...'

    # If no match is found
    return len(s1) == 0 or num > len(s1) * 1 / 2

# Function to filter out rows based on the conditions for lower and upper bounds
def filter_boundaries(data):
    filtered_data = []
    
    for entry in data:
        lower_bound = entry['Lower Bound']
        upper_bound = entry['Upper Bound']
        
        # Apply the check for both lower and upper bounds
        if not (detect_corresponding_pairs(lower_bound, upper_bound)):
        #if not (is_all_zeros(lower_bound) and is_all_fs(upper_bound)):
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
