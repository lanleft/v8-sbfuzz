import os
import subprocess
import concurrent.futures

# Define the directory to search
directory = "./test/mjsunit"
flag_prefix = "// Flags:"
js2js_maker = "popCount"
timeout_seconds = 2
max_workers = os.cpu_count()  # Use the number of CPU cores

# Function to find all regress.*.js files in the directory
def find_regress_files(directory):
    regress_files = []
    for root, _, files in os.walk(directory):
        for file in files:
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
    command = ["./out/release/d8"] + ["--test", "test/mjsunit/mjsunit.js"] + [filepath] + flags
    print(" ".join(command))
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=timeout_seconds)
        return filepath, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return filepath, "Timeout expired after 2 seconds.", ""

# Function to read the list of regress files from a file
def read_regress_files(file_path):
    with open(file_path, 'r') as file:
        return [line.strip() for line in file.readlines() if line.strip()]

# Function to process a single file
def process_file(filepath):
    try:
        content = read_file_content(filepath)
        flags = extract_flags(content)
        print(f"Running: {filepath} with flags: {' '.join(flags)}")
        filepath, stdout, stderr = run_testcase(filepath, flags)
        return filepath, stdout, stderr
    except Exception as e:
        return filepath, "", f"Error running: {filepath}\n{str(e)}"

# Main script
if __name__ == "__main__":
    regress_files = find_regress_files(directory)
    # regress_files = read_regress_files("regress_files.txt")
    js2js_tc = []
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
        future_to_file = {executor.submit(process_file, filepath): filepath for filepath in regress_files}
        
        for future in concurrent.futures.as_completed(future_to_file):
            filepath = future_to_file[future]
            try:
                filepath, stdout, stderr = future.result()
                if js2js_maker in stdout:
                    js2js_tc.append(filepath)
                
                # Uncomment these lines if you want to print output for each file
                # if stdout:
                #     print(f"Output for {filepath}:\n{stdout}")
                # if stderr:
                #     print(f"Errors for {filepath}:\n{stderr}")
            except Exception as exc:
                print(f'{filepath} generated an exception: {exc}')

    # Print or save the list of test cases that contain the js2js marker
    print("\nTest cases containing the js2js marker:")
    for testcase in js2js_tc:
        # print command to run the test case
        
        print(testcase)

    # Optionally, save to a file
    with open("js2js_tc.txt", "w") as f:
        for testcase in js2js_tc:
            f.write(testcase + "\n")
            
# Test cases containing the js2js marker:
# ./test/mjsunit/wasm/type-reflection-with-mv.js
# ./test/mjsunit/wasm/js-to-js.js
# ./test/mjsunit/wasm/type-reflection.js
