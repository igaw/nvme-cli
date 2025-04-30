#!/usr/bin/bash

# Global variables
dargs="--dry-run -vv /dev/nvme0n1"
filter="grep -E -e "^opcode" -e "^flags" -e "^cdw[0-9]+""

# Helper function to execute nvme commands
execute_nvme_command() {
  local cmd="$1"
  local full_command="../.build/nvme ${cmd} ${dargs}"

  ${full_command} | ${filter}
}

# Helper function to execute and record nvme commands
execute_and_record() {
  local cmd="$1"
  local file_name="$2"
  if [ -z "$file_name" ]; then
    echo "Error: file_name is empty in execute_and_record"
    return 1
  fi
  local output=$(execute_nvme_command "$cmd")

  echo "${output}" > "${file_name}"
  echo "${output}"
}

# Helper function to execute and compare nvme commands with recorded output
execute_and_compare() {
  local cmd="$1"
  local file_name="$2"
   if [ -z "$file_name" ]; then
    echo "Error: file_name is empty in execute_and_compare"
    return 1
  fi

  local output=$(execute_nvme_command "$cmd")
  local recorded_output=$(cat "${file_name}")

  # Compare the output with the recorded output
  if [[ "${output}" == "${recorded_output}" ]]; then
    echo "Output matches recorded output in ${file_name}"
  else
    echo "Output MISMATCH in ${file_name}!"
    echo "Expected:"
    echo "${recorded_output}"
    echo "Actual:"
    echo "${output}"
    return 1 # Return non-zero to indicate failure
  fi
  # print the output
  echo "${output}"
}

# Helper function to generate command name and filename
generate_file_info() {
  local cmd="$1"
  # Extract the command name for the filename
  # Escape special characters for use in filename
  local command_name=$(echo "$cmd" | sed 's/[^a-zA-Z0-9_-]/_/g')
  local file_name="${record_dir}/${command_name}.txt"
  echo "$file_name"
}

# Main script logic
mode="$1"
shift # Remove the mode argument, so the remaining arguments are correctly parsed.


# Create directory if it does not exist
record_dir="recordings"
if [ "$mode" == "record" ] || [ "$mode" == "replay" ]; then
  mkdir -p "$record_dir"
fi

# Define the nvme commands  in a table
nvme_commands=(
  "id-ctrl"
  "id-ns"
  "create-ns --nsze-si=1G --flbas 1" # Added arguments.
  "create-ns --nsze-si=2G --flbas 1" # Added another create-ns command
)

case "$mode" in
  "record")
    echo "Running in record mode"
    for cmd in "${nvme_commands[@]}"; do
      file_name=$(generate_file_info "$cmd")
      execute_and_record "$cmd" "$file_name"
    done
    ;;
  "replay")
    echo "Running in replay mode"
    for cmd in "${nvme_commands[@]}"; do
      file_name=$(generate_file_info "$cmd")
      execute_and_compare "$cmd" "$file_name"
    done
    ;;
  "")
    echo "Usage: $0 <mode> [options]"
    echo "  <mode>: record | replay"
    echo "  record    - Executes commands and saves the output."
    echo "  replay    - Executes commands and compares with saved output."
    exit 1
    ;;
  *)
    echo "Error: Invalid mode '$mode'"
    echo "Usage: $0 <mode> [options]"
    echo "  <mode>: record | replay"
    echo "  record    - Executes commands and saves the output."
    echo "  replay    - Executes commands and compares with saved output."
    exit 1
    ;;
esac

exit 0
