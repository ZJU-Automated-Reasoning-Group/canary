import pandas as pd

# Load the data (assuming the data is in a CSV file)
df = pd.read_csv('/home/zarin/z_solver/results/blcmpr.csv')

# List of substrings to remove from filenames for normalization
substrings_to_remove = ['/16lia', '/16nia', '/16bits', '/64lia', '/64nia', '/64bits', '/32lia', '/32nia', '/32bits', '/128lia', '/128nia', '/128bits', '/256lia', '/256nia', '/256bits']

# Step 1: Normalize the filenames by removing the specified substrings
def normalize_filename(filename):
    for substring in substrings_to_remove:
        filename = filename.replace(substring, '')
    return filename

df = df[(df['Time Cost'] <= 60)]

df['Normalized File'] = df['File'].apply(normalize_filename)

# Step 2: Count occurrences of each normalized filename
file_counts = df['Normalized File'].value_counts()

print(file_counts.size)

# Step 3: Get filenames that appear at least 3 timess
common_files = file_counts[file_counts >= 6].index.tolist()

# Step 5: Additional filters (optional)
filtered_df = df[
    df['Normalized File'].isin(common_files) &
    #df['File'].str.contains('128bits') &  # Assuming you want files that contain '128bits'
    (df['Bad Solve'] == 0)
]

# Step 6: Group by 'Method' and calculate the required statistics
stats = filtered_df.groupby('Method').agg(
    num_files=('File', 'count'),  # Count the number of files for each method
    avg_time=('Time Cost', 'mean'),  # Calculate the average time cost for each method
    total_time=('Time Cost', 'sum'),  # Calculate the total time cost for each method
    total_checks=('Calls', 'sum'),
    avg_checks=('Calls', 'mean')
)
stats['avg_time_per_check'] = stats['total_time'] / stats['total_checks']
stats['avg_time_per_check'] = stats['avg_time_per_check'].fillna(0)  # Replace NaN with 0 for divide-by-zero cases

# Step 7: Display the results
print(stats)
