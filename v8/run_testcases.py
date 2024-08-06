import os
import subprocess

# Define the directory to search
directory = "./test/mjsunit"
flag_prefix = "// Flags:"
js2js_maker = "popCount"
timeout_seconds = 2

# Function to find all regress.*.js files in the directory
def find_regress_files(directory):
    regress_files = []
    for root, _, files in os.walk(directory):
        for file in files:
            # if file.startswith("regress") and file.endswith(".js"):
            if file.endswith(".js"):
                regress_files.append(os.path.join(root, file))
    return regress_files

# Function to read the content of a file
def read_file_content(filepath):
    with open(filepath, 'r') as file:
        return file.readlines()

# Function to extract flags from the file content
def extract_flags(file_content):
    for line in file_content:
        if line.startswith(flag_prefix):
            flags = line[len(flag_prefix):].strip().split()
            return flags
    return []

# Function to run the test case
def run_testcase(filepath, flags):
    command = ["./out/release/d8"] + flags + [filepath]
    # print string command
    print(" ".join(command))
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=timeout_seconds)
        return result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return "Timeout expired after 2 seconds.", ""

# Function to read the list of regress files from a file
def read_regress_files(file_path):
    with open(file_path, 'r') as file:
        return [line.strip() for line in file.readlines() if line.strip()]

# Main script
if __name__ == "__main__":
    regress_files = find_regress_files(directory)
    # regress_files = read_regress_files("regress_files.txt")
    js2js_tc = []
    
    for filepath in regress_files:
        try:
            content = read_file_content(filepath)
            flags = extract_flags(content)
            
            print(f"Running: {filepath} with flags: {' '.join(flags)}")
            stdout, stderr = run_testcase(filepath, flags)
            
            if js2js_maker in stdout:
                js2js_tc.append(filepath)
            
            if stdout:
                print(f"Output:\n{stdout}")
            if stderr:
                print(f"Errors:\n{stderr}")
        except Exception as e:
            print(f"Error running: {filepath}")
    # Print or save the list of test cases that contain the js2js marker
    print("\nTest cases containing the js2js marker:")
    for testcase in js2js_tc:
        print(testcase)

    # Optionally, save to a file
    with open("js2js_tc.txt", "w") as f:
        for testcase in js2js_tc:
            f.write(testcase + "\n")