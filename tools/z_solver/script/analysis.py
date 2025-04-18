import pandas as pd
import matplotlib.pyplot as plt
from matplotlib import font_manager
import seaborn as sns
import argparse
import csv
import numpy as np
import re

plt.rcParams['font.family'] = 'Times New Roman'
methods = ['omtfix', 'binfix', 'bi', 'decr', 'test']
method_legend = ['CIBII(OMT)', 'CIBII(BS)', 'CIBII(Bi)', 'EFBII(Lin)', 'EFBII(BinLift)']

# Function to read CSV data
def read_csv(file_path):
    data = []
    with open(file_path, 'r') as file:
        reader = csv.DictReader(file)
        for row in reader:
            data.append(row)
    return data

# Function to analyze data and convert it into a pandas DataFrame
def prepare_data_for_plotting(file_path):
    data = read_csv(file_path)
    
    # Convert data into a pandas DataFrame
    df = pd.DataFrame(data)
    
    # Convert 'Time Cost' and 'Calls' to numeric types
    df['Time Cost'] = pd.to_numeric(df['Time Cost'], errors='coerce')
    df['Calls'] = pd.to_numeric(df['Calls'], errors='coerce')

    return df

def plot_time_cost(df, method1, method2, save=False, save_path=None):
    
    # Get the corresponding legend labels for method1 and method2
    method1_label = method_legend[methods.index(method1)]
    method2_label = method_legend[methods.index(method2)]
    
    # Filter the data for Method1 and Method2
    df_method1 = df[df['Method'] == method1]
    df_method2 = df[df['Method'] == method2]
    
    # Merge the data
    merged_df = pd.merge(df_method1[['File', 'Time Cost', 'Calls']], 
                         df_method2[['File', 'Time Cost', 'Calls']],
                         how='outer', on='File',
                         suffixes=(f'_{method1}', f'_{method2}'))

    # Adjust 'Time Cost' to 60 where it's missing after filling from either table
    merged_df[f'Time Cost_{method1}'] = np.clip(merged_df[f'Time Cost_{method1}'].fillna(30), 0, 30)
    merged_df[f'Time Cost_{method2}'] = np.clip(merged_df[f'Time Cost_{method2}'].fillna(30), 0, 30)
    
    # Set up the plot layout with 1 subplot (Time Cost plot)
    fig, ax = plt.subplots(figsize=(5, 5))
    
    # Plot the Time Cost comparison (Method 1 vs Method 2)
    sns.scatterplot(data=merged_df, x=f'Time Cost_{method1}', y=f'Time Cost_{method2}', marker='x', color='blue', s=160, ax=ax)
    
    ax.set_xlabel(f'Time Cost of {method1_label}', fontsize=20, family='Times New Roman')
    ax.set_ylabel(f'Time Cost of {method2_label}', fontsize=20, family='Times New Roman')

    # Set axis limits
    ax.set_xlim([0, 30])
    ax.set_ylim([0, 30])

    # Diagonal and other lines
    ax.plot([0, ax.get_xlim()[1]], [0, ax.get_xlim()[1]], color='gray', linestyle='--')
    x_vals = np.array([0, ax.get_xlim()[1]])  # x values for the line
    y_vals_2_1 = 2 * x_vals  # y = 2 * x
    ax.plot(x_vals, y_vals_2_1, color='gray', linestyle='--')
    y_vals_1_2 = 0.5 * x_vals  # y = 0.5 * x
    ax.plot(x_vals, y_vals_1_2, color='gray', linestyle='--')

    plt.tight_layout()  # Adjust layout to avoid overlap
    
    # Save the figure if requested
    if save:
        if save_path is None:
            save_path = f"time_cost_comparison_{method1}_{method2}.png"  # Default name if no save_path is provided
        fig.savefig(save_path)
        print(f"Time Cost plot saved as {save_path}")
    else:
        plt.show()

def plot_calls(df, method1, method2, save=False, save_path=None):
    
    # Get the corresponding legend labels for method1 and method2
    method1_label = method_legend[methods.index(method1)]
    method2_label = method_legend[methods.index(method2)]
    
    # Filter the data for Method1 and Method2
    df_method1 = df[df['Method'] == method1]
    df_method2 = df[df['Method'] == method2]
    
    # Merge the data
    merged_df = pd.merge(df_method1[['File', 'Time Cost', 'Calls']], 
                         df_method2[['File', 'Time Cost', 'Calls']],
                         how='outer', on='File',
                         suffixes=(f'_{method1}', f'_{method2}'))
    
    # Set up the plot layout with 1 subplot (Calls plot)
    fig, ax = plt.subplots(figsize=(10, 10))
    
    # Plot the Calls comparison (Method 1 vs Method 2)
    sns.scatterplot(data=merged_df, x=f'Calls_{method1}', y=f'Calls_{method2}', marker='x', markersize=8, color='red', ax=ax)
    
    ax.set_xlabel(f'Number of Solver Calls of {method1_label}', fontsize=14, family='Times New Roman')
    ax.set_ylabel(f'Number of Solver Calls of {method2_label}', fontsize=14, family='Times New Roman')

    # Equal scaling for both axes and draw the diagonal line
    ax.set_xlim([0, max(merged_df[f'Calls_{method1}'].max(), merged_df[f'Calls_{method2}'].max())])
    ax.set_ylim([0, max(merged_df[f'Calls_{method1}'].max(), merged_df[f'Calls_{method2}'].max())])
    
    # Diagonal and other lines
    ax.plot([0, ax.get_xlim()[1]], [0, ax.get_xlim()[1]], color='gray', linestyle='--')
    x_vals = np.array([0, ax.get_xlim()[1]])  # x values for the line
    y_vals_2_1 = 2 * x_vals  # y = 2 * x
    ax.plot(x_vals, y_vals_2_1, color='gray', linestyle='--')
    y_vals_1_2 = 0.5 * x_vals  # y = 0.5 * x
    ax.plot(x_vals, y_vals_1_2, color='gray', linestyle='--')

    plt.tight_layout()  # Adjust layout to avoid overlap
    
    # Save the figure if requested
    if save:
        if save_path is None:
            save_path = f"calls_comparison_{method1}_{method2}.png"  # Default name if no save_path is provided
        fig.savefig(save_path)
        print(f"Calls plot saved as {save_path}")
    else:
        plt.show()

def plot_cactus_time_cost(df, save=False, save_path=None):
    """
    Draw a line chart comparing all methods detected in the dataset, sorted by time cost for each method.
    
    Args:
        df (DataFrame): The dataframe containing the time cost data.
        save (bool): Whether to save the plot to a file.
        save_path (str): Path to save the plot, if saving is enabled.
    """
    # List of methods to be plotted
    methods = ['omtfix', 'binfix', 'bi', 'decr', 'test']
    #methods = ['test', 'binlift-+', 'binlift++', 'binlift+', 'binlift-', 'binlift']
    
    # Corresponding legend labels for the methods
    method_legend = ['CIBII(OMT)', 'CIBII(BS)', 'CIBII(Bi)', 'EFBII(Lin)', 'EFBII(BinLift)']
    #method_legend = [ 'EFBII(BinLift)', 'EFBII(BinLift-NoRM)', 'EFBII(BinLift-NoHD)', 'EFBII(BinLift-RM)', 'EFBII(BinLift-BB)', 'EFBII(BinLift-Base)']
    
    # Create a dictionary that maps methods to legend labels
    method_to_legend = dict(zip(methods, method_legend))
    
    # List of markers for each method
    markers = ['s', 'v', 'o', '^', 'D', 'X']
    
    # Create the plot
    fig, ax = plt.subplots(figsize=(10, 6))

    # Loop through each method detected in the dataset
    for i, method in enumerate(methods):
        # Filter data for the current method
        df_method = df[df['Method'] == method]

        # Sort the data by 'Time Cost' to arrange points in ascending order of time cost
        df_method_sorted = df_method[['File', 'Time Cost']].sort_values(by='Time Cost')

        # Calculate cumulative time cost
        df_method_sorted['Cumulative Time Cost'] = df_method_sorted['Time Cost'].cumsum()

        # X-axis range (1, 2, 3, ...)
        x_values = range(1, len(df_method_sorted) + 1)

        # Plot the cumulative time cost as a line
        ax.plot(
            x_values,  # X-axis: File count
            df_method_sorted['Cumulative Time Cost'],  # Y-axis: Cumulative time cost
            label=method_to_legend[method],  # Use the corresponding legend label
            marker=markers[i],  # Add a unique marker for each method
            markersize=6,  # Set marker size
            markevery=20  # Mark every 10th point
        )

    # Add titles and labels
    ax.set_xlabel('Solved Instances', fontsize=14)
    ax.set_ylabel('Time (Seconds)', fontsize=14)
    plt.legend(title='Algorithms')

    # Optional: Add a grid for better readability
    ax.grid(True)

    # Tight layout for better spacing
    plt.tight_layout()

    # Save the plot if required
    if save:
        if save_path is None:
            save_path = "line_cactus_time_cost_comparison_sorted_by_time.png"
        fig.savefig(save_path)
        print(f"Cactus plot saved to {save_path}")
    else:
        plt.show()
        
def plot_accumulated_solved(df, save=False, save_path=None):
    """
    Plots an 'accumulated solved instances per time bin' vs time plot, grouped by methods.
    
    Args:
        df (DataFrame): The input dataframe containing 'Method', 'Time Cost', and 'Bad Solve' columns.
        save (bool): Whether to save the plot to a file.
        save_path (str): Path to save the plot, if saving is enabled.
    """
    
    # Filter out rows where time cost is 0 (invalid entries)
    df = df[df['Time Cost'] > 0]

    # Round the 'Time Cost' up to the nearest 0.5 second (upper estimate)
    df['Time Bin'] = np.ceil(df['Time Cost'] * 2) / 2  # Round up to nearest 0.5s
    
    # Handle the special case where time bin starts from 0
    # Ensure that if the time is zero, it belongs to the 0s bin
    df.loc[df['Time Bin'] == 0, 'Time Bin'] = 0.5
    
    # Group by 'Method' and 'Time Bin', and count the number of solved instances per bin
    second_df = df.groupby(['Method', 'Time Bin'], as_index=False)['Time Cost'].count()
    second_df.rename(columns={'Time Cost': 'Solved Instances'}, inplace=True)

    # Create the plot
    plt.figure(figsize=(10, 6))
    
    # List of markers for each method's line
    markers = ['o', 's', 'D', '^', 'v']  # Circle, square, diamond, triangle-up, triangle-down
    
    # Loop through each method and plot with different markers
    for idx, method in enumerate(methods):  # Use 'methods' list for consistent method order
        method_data = second_df[second_df['Method'] == method]
        
        # Sort by time bin
        method_data = method_data.sort_values('Time Bin')
        
        # Create a cumulative sum of solved instances for each time bin
        method_data['Cumulative Solved'] = method_data['Solved Instances'].cumsum()

        # Create a complete range of time bins for 0.5s intervals (e.g., 0.5, 1.0, 1.5, ...), up to 60s
        all_time_bins = np.arange(0.5, 60.5, 0.5)  # Extend to 60 seconds (60.0s included)

        # Reindex the data to include all 0.5s intervals and fill missing values by carrying forward the last value
        method_data = method_data.set_index('Time Bin').reindex(all_time_bins, method='ffill').reset_index()
        method_data.rename(columns={'index': 'Time Bin'}, inplace=True)

        # Add the (0s, 0) bin to the beginning of the data
        zero_data = pd.DataFrame({'Method': [method], 'Time Bin': [0], 'Solved Instances': [0], 'Cumulative Solved': [0]})
        method_data = pd.concat([zero_data, method_data], ignore_index=True)

        # Plot the cumulative number of solved instances with a unique marker for each method
        # Plot markers only at every 3 seconds
        sns.lineplot(data=method_data, x='Time Bin', y='Cumulative Solved', label=method_legend[idx], marker=markers[idx], markersize=6)
    
    # Set plot labels and title with Arial font
    plt.xlabel('Time (seconds)', fontsize=14, family='Times New Roman')
    plt.ylabel('Accumulated Solved Instances', fontsize=14, family='Times New Roman')

    # Set font properties for the legend using FontProperties
    font_prop = font_manager.FontProperties(family='Times New Roman', weight='normal', size=12)
    
    # Move the legend to the lower-right corner and set font to Arial
    plt.legend(title='Algorithms', loc='lower right', fontsize=12, title_fontsize=14, prop=font_prop)
    
    # Optional: Add a grid for better readability
    plt.grid(True)
    
    # Tight layout for better spacing
    plt.tight_layout()

    # Save the plot if required
    if save:
        if save_path is None:
            save_path = "accumulated_solved_instances_time_plot.png"
        plt.savefig(save_path)
        print(f"Plot saved to {save_path}")
    else:
        plt.show()
        
def plot_time_cost_by_bit(df, method, save=False, save_path=None):
    """
    Draws a scatter plot comparing the Time Cost for files categorized by 32bit, 64bit, and 128bit.
    
    Args:
        df (DataFrame): The dataframe containing the time cost data.
        save (bool): Whether to save the plot to a file.
        save_path (str): Path to save the plot, if saving is enabled.
    """
    # Extract the bit categories from the filenames
    df['Bit Category'] = df['File'].apply(lambda x: '32bit' if '32bit' in x else ('64bit' if '64bit' in x else ('128bit' if '128bit' in x else 'Other')))
    
    # Filter out rows where the 'Bit Category' is 'Other' and include only rows where Method is 'test'
    df_filtered = df[(df['Bit Category'].isin(['32bit', '64bit', '128bit'])) & (df['Method'] == method)]
    
    # Create the plot
    fig, ax = plt.subplots(figsize=(4, 5))

    # Define a color palette for each category
    palette = {'32bit': 'red', '64bit': 'green', '128bit': 'blue'}

    # Create a scatter plot for 'Time Cost' by 'Bit Category'
    sns.scatterplot(x='Bit Category', y='Time Cost', data=df_filtered, ax=ax, hue='Bit Category', palette=palette, marker='x', s=100, legend=False)

    # Set the labels and title
    ax.set_xlabel('Bit Category', fontsize=14)
    ax.set_ylabel('Time Cost (Seconds)', fontsize=14)

    # Fix the y-axis to be between 0 and 60 seconds
    ax.set_ylim(0, 40)

    # Optional: Add a grid for better readability
    ax.grid(True)

    # Tight layout for better spacing
    plt.tight_layout()

    # Save the plot if required
    if save:
        if save_path is None:
            save_path = "time_cost_by_bit_category_points.png"
        fig.savefig(save_path)
        print(f"Plot saved to {save_path}")
    else:
        plt.show()
        
def compare_bounds(row):
    """
    Compares the Lower Bound and Upper Bound of each row and finds the bit position where they differ.
    Assumes the 'Lower Bound' and 'Upper Bound' columns are in the form of ranges of hexadecimal strings.
    
    Args:
        row (Series): DataFrame row with ['Lower Bound', 'Upper Bound'] columns.
    
    Returns:
        differ_bit (int): The bit position where the bounds differ.
    """
    # Extract the lower and upper bound strings
    lower_bound_str = row['Lower Bound']
    upper_bound_str = row['Upper Bound']
    
    # Use regular expressions to find all hexadecimal values in the range
    lower_bound_vals = re.findall(r'[0-9a-fA-F]+', lower_bound_str)
    upper_bound_vals = re.findall(r'[0-9a-fA-F]+', upper_bound_str)

    # Check if the bounds are valid
    if not lower_bound_vals or not upper_bound_vals:
        return None

    # Compare the hex values byte by byte (from most significant to least significant)
    for lower_val, upper_val in zip(lower_bound_vals, upper_bound_vals):
        # Convert hex values to integers
        lower_int = int(lower_val, 16)
        upper_int = int(upper_val, 16)
        
        # Compare the binary representations of the two values
        xor_result = lower_int ^ upper_int
        
        # If there's a difference, find the position of the first differing bit
        if xor_result != 0:
            # Find the position of the first differing bit
            differ_bit = xor_result.bit_length()
            return differ_bit
    
    # If the bounds are exactly the same, return None or 0
    return 0

def plot_time_cost_by_diff_bit(df, method, save=False, save_path=None):
    """
    Plot a scatter plot of Time Cost against differing bit position, for the 'test' method only.
    
    Args:
        df (DataFrame): The dataframe containing the time cost and bound data.
        save (bool): Whether to save the plot to a file.
        save_path (str): Path to save the plot, if saving is enabled.
    """
    # Filter the data for the 'test' method
    df_test = df[df['Method'] == method]
    
    # Apply the compare_bounds function to each row to calculate the differing bit position
    df_test['Differ Bit'] = df_test.apply(compare_bounds, axis=1)

    # Filter out rows with no differing bit (if any)
    df_test = df_test.dropna(subset=['Differ Bit'])

    # Create the plot
    fig, ax = plt.subplots(figsize=(4, 5))
    
    # Define a single color for the 'test' method
    color = 'blue'
    
    # Create scatter plot
    sns.scatterplot(x='Differ Bit', y='Time Cost', data=df_test, 
                    color=color, marker='x', s=100)

    # Set the labels and title
    ax.set_xlabel('Differing Bit Position', fontsize=20)
    ax.set_ylabel('Time Cost (Seconds)', fontsize=20)
    ax.set_ylim(0, 40)
    
    # Make tick label font size larger
    ax.tick_params(axis='both', which='major', labelsize=16)  # Increase major ticks font size
    ax.tick_params(axis='both', which='minor', labelsize=12)  # (Optional) Minor ticks if needed
    
    # Optional: Add a grid for better readability
    ax.grid(True)

    # Adjust layout
    plt.tight_layout()

    # Save the plot if required
    if save:
        if save_path is None:
            save_path = "time_cost_vs_differing_bit_test_method.png"
        fig.savefig(save_path)
        print(f"Plot saved to {save_path}")
    else:
        plt.show()

def plot_calls_by_bit(df, method, save=False, save_path=None):
    """
    Draws a scatter plot comparing the number of 'Calls' for files categorized by 32bit, 64bit, and 128bit.

    Args:
        df (DataFrame): The dataframe containing the call data.
        save (bool): Whether to save the plot to a file.
        save_path (str): Path to save the plot, if saving is enabled.
    """
    # Extract the bit categories from the filenames
    df['Bit Category'] = df['File'].apply(
        lambda x: '32bit' if '32bit' in x else ('64bit' if '64bit' in x else ('128bit' if '128bit' in x else 'Other'))
    )

    # Filter out rows where the 'Bit Category' is 'Other' and include only rows where Method is 'test'
    df_filtered = df[(df['Bit Category'].isin(['32bit', '64bit', '128bit'])) & (df['Method'] == method)]

    # Create the plot
    fig, ax = plt.subplots(figsize=(4, 5))

    # Define a color palette for each category
    palette = {'32bit': 'red', '64bit': 'green', '128bit': 'blue'}

    # Create a scatter plot for 'Calls' by 'Bit Category'
    sns.scatterplot(x='Bit Category', y='Calls', data=df_filtered, ax=ax, hue='Bit Category', 
                    palette=palette, marker='x', s=100, legend=False)

    # Set the labels and title
    ax.set_xlabel('Bit Category', fontsize=14)
    ax.set_ylabel('Number of Calls', fontsize=14)

    # Optional: Fix the y-axis range if needed (e.g., 0 to 1000)
    ax.set_ylim(0, df_filtered['Calls'].max() * 1.1)

    # Optional: Add a grid for better readability
    ax.grid(True)

    # Tight layout for better spacing
    plt.tight_layout()

    # Save the plot if required
    if save:
        if save_path is None:
            save_path = "calls_by_bit_category_points.png"
        fig.savefig(save_path)
        print(f"Plot saved to {save_path}")
    else:
        plt.show()

# Main function
if __name__ == "__main__":
    # Set up argument parser
    parser = argparse.ArgumentParser(description='Plot comparisons of Time Cost and Calls between methods')
    parser.add_argument('-F', '--file', type=str, required=True, help="Source file")
    parser.add_argument('--save', action='store_true', help="Save the plot to a file")
    parser.add_argument('--path', type=str, help="Path to save the plot")
    parser.add_argument('--plot', type=int, required=True, help="Choose plot to save")
    parser.add_argument('--m1', type=str, help="Method1")
    parser.add_argument('--m2', type=str, help="Method2")
    args = parser.parse_args()
    
    # Path to the CSV file
    file_path = args.file  # Update this path
    
    # Prepare the data for plotting
    df = prepare_data_for_plotting(file_path)
    
    # Plot and save the selected plot type
    if args.plot == 0:
        plot_time_cost(df, args.m1, args.m2, save=args.save, save_path=args.path)
    elif args.plot == 1:
        plot_calls(df, args.m1, args.m2, save=args.save, save_path=args.path)
    elif args.plot == 2:
        plot_cactus_time_cost(df, save=args.save, save_path=args.path)
    elif args.plot == 3:
        plot_accumulated_solved(df, save=args.save, save_path=args.path)
    elif args.plot == 4:
        plot_time_cost_by_bit(df, args.m1, save=args.save, save_path=args.path)
    elif args.plot == 5:
        plot_time_cost_by_diff_bit(df, args.m1, save=args.save, save_path=args.path)
    elif args.plot == 6:
        plot_calls_by_bit(df, args.m1, save=args.save, save_path=args.path)
