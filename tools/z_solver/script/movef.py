import os
import shutil
import pandas as pd

def copy_files_to_directory(df, target_directory):
    """
    Copies all mentioned files from the 'File' column in the DataFrame to a specified directory,
    replacing '/data' with '/home' in the file paths.
    
    Args:
        df (DataFrame): The dataframe containing the file paths.
        target_directory (str): The target directory where files should be copied.
    """
    # Ensure the target directory exists, create if not
    if not os.path.exists(target_directory):
        os.makedirs(target_directory)
    
    # Iterate over the file paths in the 'File' column
    for file_path in df['File']:
        # Replace '/data' with '/home' in the file path
        modified_file_path = file_path.replace('/data/zhr', '/home/zarin')
        
        # Ensure the modified file exists before attempting to copy
        if os.path.exists(modified_file_path):
            # Get the base file name (just the file name, no path)
            file_name = os.path.basename(modified_file_path)
            
            # Construct the destination path
            destination_path = os.path.join(target_directory, file_name)
            
            # Copy the file to the target directory
            shutil.copy(modified_file_path, destination_path)
            print(f"Copied: {modified_file_path} to {destination_path}")
        else:
            print(f"File not found: {modified_file_path}")

# Example usage:
# Assuming `df` is your DataFrame and `target_directory` is where you want to copy the files
df = pd.read_csv('/home/zarin/z_solver/results/allnew.csv_new.csv')  # Load the CSV into a DataFrame
target_directory = '/home/zarin/z_solver/novesafe'  # Set the path to your target directory
copy_files_to_directory(df, target_directory)

