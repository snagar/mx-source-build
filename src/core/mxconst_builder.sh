#!/bin/bash

# --- Configuration ---
class_name="mxconst"
template_file="mxconst.template"
header_file="mxconst.h"
#cpp_file="mxconst.cpp"
start_time=$(date +%s)

# --- Functions ---

write_mxconst_h_footer() {
  echo -e "\n\n\t};// end class \n} // missionx namespace" >> "$header_file"
  echo -e "\n#endif //MXCONST_H" >> "$header_file"
}


write_header_line_constexpr() {
  local definition="$1"
  local datatype="$2"
  local name="$3"
  local param_value="$4"
  local comment="$5"

  name="$(echo "$name" | xargs)"

  local header_line="$definition $datatype $name = $param_value"
  local trimmed_value=$(echo "$param_value" | sed 's/[[:space:]]*$//')
  local semicolon=""
  if [[ ! "$trimmed_value" == *';' ]]; then
    semicolon=";"
  fi

  # Remove " const " from header line only if "name" does not start with "*" (pointer)
  if [[ ! "$name" =~ ^(\*).* ]]; then
    header_line=${header_line// const /}
  fi

  local full_line="$header_line$semicolon"
  if [[ -n "$comment" ]]; then
    full_line="$full_line // $comment"
  fi
  echo "$full_line" >> "$header_file"
}

write_header_struct() {
  local datatype="$1"
  local name="$2"
  local param_value="$3"

  local struct_name=""
  local getter_name=""
  local trimmed_value=""
  name="$(echo "$name" | xargs)"
  struct_name="st_$(echo "$name" | tr '[:upper:]' '[:lower:]' | xargs)"
#  getter_name="get_$(echo "$name" | tr '[:upper:]' '[:lower:]' | xargs)"
  getter_name="get_$(echo "$name" | xargs)"
  trimmed_value=$(echo "$param_value" | sed 's/[[:space:]]*$//')

  local semicolon=""
  if [[ ! "$trimmed_value" == *';' ]]; then
    semicolon=";"
  fi

  echo -e "// ---> $name" >> "$header_file"
  echo "typedef struct $struct_name {" >> "$header_file"
  echo "  $datatype value = $param_value$semicolon" >> "$header_file"
  echo "[[nodiscard]]  $datatype getValue() const { return value; }" >> "$header_file"
  echo "} $struct_name;" >> "$header_file"
  # Add space
  # Create the getter function
  echo -e "// -- getter -- " >> "$header_file"
  echo "static $datatype $getter_name() {" >> "$header_file"
  echo "  static const $struct_name instance;" >> "$header_file"
  echo "  return instance.getValue();" >> "$header_file"
  echo "}" >> "$header_file"
  echo -e "// $name <---\n" >> "$header_file"
}

# --- Main Script ---
main () {
echo "Starting script at: $(date)"

# Reset files
echo "Resetting file: $header_file"
> "$header_file"
#> "$cpp_file"

echo "Processing template file: $template_file"
while IFS= read -r line; do
  if [[ ! "$line" =~ ^(const|static|constexpr|inline|>).* ]]; then
#    echo "Appending to header (non-keyword): $line"
    echo "$line" >> "$header_file"
  else
    # Split the line using "^" as delimiter
    IFS="^" read -ra fields <<< "$line"

    # Check if we have at least 4 fields
    if [[ ${#fields[@]} -lt 4 ]]; then
      echo "Warning: Line '$line' has less than 4 fields. Skipping: $line"
      continue
    fi

    local definition="${fields[0]}"
    local datatype="${fields[1]}"
    local name="${fields[2]}"
    local param_value="${fields[3]}"
    local comment="${fields[@]:4}"

    # Handle constexpr
    if [[ "$definition" == *"constexpr"* ]]; then
      local modified_definition="$definition"
      if [[ ! "$definition" =~ static ]]; then
        modified_definition="static $modified_definition"
      fi
      write_header_line_constexpr "$modified_definition" "$datatype" "$name" "$param_value" "$comment"
    # Handle static (without constexpr)
    elif [[ "$definition" == *"static"* ]] && [[ ! "$definition" == *"constexpr"* ]]; then
      write_header_struct "$datatype" "$name" "$param_value"
    else
      # For other keywords (const, inline, >) just write to header
      local header_line="$definition $datatype $name"
      local semicolon=""
      local trimmed_value=$(echo "$param_value" | sed 's/[[:space:]]*$//')
      if [[ ! "$trimmed_value" == *';' ]]; then
        semicolon=";"
      fi
      local full_line="$header_line$semicolon"
      if [[ -n "$comment" ]]; then
        full_line="$full_line // $comment"
      fi
      echo "$full_line" >> "$header_file"
    fi
  fi
done < "$template_file"
}

main

write_mxconst_h_footer

# Replace "%class_name%" with "$class_name" in mxconst.h

echo "Replacing '%class_name%' with '$class_name' in $header_file"
sed -i "s/%class_name%/$class_name/g" "$header_file"
#sed -i "s/%generated%/$(date)/g" "$header_file"
sed -i "s/%generated%/$(date "+%Y %b %d, %H:%M:%S")/g" "$header_file"

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Script finished at: $(date)"
echo "Total execution time: ${duration} seconds"

exit 0
