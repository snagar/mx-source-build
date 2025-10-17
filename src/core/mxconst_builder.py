import subprocess
import datetime
import re
import sys

def write_mxconst_h_footer(header_file):
    """Writes the footer for the mxconst.h file."""
    with open(header_file, "a") as f:
        f.write("\n\n\t};// end class \n} // missionx namespace\n")
        f.write("\n#endif //MXCONST_H\n")

def write_header_line_constexpr(header_file, definition, datatype, name, param_value, comment):
    """Writes a constexpr line to the header file.

    Args:
        header_file: The path to the header file.
        definition: The definition (e.g., "static constexpr").
        datatype: The data type.
        name: The variable name.
        param_value: The parameter value.
        comment: The comment string.
    """
    name = name.strip()
    datatype = datatype.strip()
    header_line = f"{definition} {datatype} {name} = {param_value}"
    trimmed_value = param_value.rstrip()
    semicolon = ";" if not trimmed_value.endswith(";") else ""

    # Remove " const " from header line only if "name" does not start with "*" (pointer)
    if not name.startswith("*") and not datatype.endswith("*"):
        header_line = header_line.replace(" const ", " ")
        # print(f'Name: {name}, datatype: {datatype}')  # debug

    full_line = f"{header_line}{semicolon}"
    if comment:
        full_line += f" // {comment}"
    with open(header_file, "a") as f:
        f.write(f"{full_line}\n")


def write_header_struct(header_file, datatype, name, param_value):
    """Writes a struct definition and getter function to the header file.

    Args:
        header_file: The path to the header file.
        datatype: The data type.
        name: The variable name.
        param_value: The parameter value.
    """
    name = name.strip()
    struct_name = f"st_{name.lower()}"
    getter_name = f"get_{name}"
    trimmed_value = param_value.rstrip()
    semicolon = ";" if not trimmed_value.endswith(";") else ""

    with open(header_file, "a") as f:
        f.write(f"// ---> {name}\n")
        f.write(f"typedef struct {struct_name} {{\n")
        f.write(f"  {datatype} value = {param_value}{semicolon}\n")
        f.write(f"[[nodiscard]]  {datatype} getValue() const {{ return value; }}\n")
        f.write(f"}} {struct_name};\n")
        # Add space
        # Create the getter function
        f.write(f"// -- getter -- \n")
        f.write(f"static {datatype} {getter_name}() {{\n")
        f.write(f"  static const {struct_name} instance;\n")
        f.write(f"  return instance.getValue();\n")
        f.write(f"}}\n")
        f.write(f"// {name} <---\n")


def main(template_file, header_file):
    """Main function to process the template file and generate the header file."""
    print(f"Starting script at: {datetime.datetime.now()}")

    # Reset files
    print(f"Resetting file: {header_file}")
    with open(header_file, "w") as f:
        f.write("")  # Clear the file

    print(f"Processing template file: {template_file}")
    with open(template_file, "r") as infile:
        for line in infile:
            line = line.strip()
            if not re.match(r"^(const|static|constexpr|inline|>)", line):
                with open(header_file, "a") as outfile:
                    outfile.write(f"{line}\n")
            else:
                # Split the line using "^" as delimiter
                fields = line.split("^")

                # Check if we have at least 4 fields
                if len(fields) < 4:
                    print(f"Warning: Line '{line}' has less than 4 fields. Skipping: {line}")
                    continue

                definition, datatype, name, param_value = fields[:4]
                comment = "^".join(fields[4:])  # Rejoin any remaining parts of the comment

                # Handle constexpr
                if "constexpr" in definition:
                    modified_definition = definition
                    if "static" not in definition:
                        modified_definition = f"static {modified_definition}"
                    write_header_line_constexpr(header_file, modified_definition, datatype, name, param_value, comment)
                # Handle static (without constexpr)
                elif "static" in definition and "constexpr" not in definition:
                    write_header_struct(header_file, datatype, name, param_value)
                else:
                    # For other keywords (const, inline, >) just write to header
                    header_line = f"{definition} {datatype} {name}"
                    semicolon = ";" if not param_value.rstrip().endswith(";") else ""
                    full_line = f"{header_line}{semicolon}"
                    if comment:
                        full_line += f" // {comment}"
                    with open(header_file, "a") as outfile:
                        outfile.write(f"{full_line}\n")

if __name__ == "__main__":
    class_name = "mxconst"
    template_file = "mxconst.template"
    header_file = "mxconst.h"
    # cpp_file = "mxconst.cpp" # Not used in the original script

    start_time = datetime.datetime.now()

    main(template_file, header_file)
    write_mxconst_h_footer(header_file)

    # Replace "%class_name%" with "$class_name" in mxconst.h
    print(f"Replacing '%class_name%' with '{class_name}' in {header_file}")
    try:
        # Use a temporary file to avoid potential issues with modifying the file in-place
        with open(header_file, "r") as infile, open("temp.h", "w") as outfile:
            for line in infile:
                modified_line = line.replace("%class_name%", class_name)
                outfile.write(modified_line)
        import os
        os.replace("temp.h", header_file)  # Rename the temporary file

        # Use datetime.datetime.now() for more flexibility in formatting
        generated_time = datetime.datetime.now().strftime("%Y %b %d, %H:%M:%S")
        script_name = os.path.basename(__file__)
        with open(header_file, "r") as infile, open("temp.h", "w") as outfile:
            for line in infile:
                modified_line = line.replace("%script_name%", script_name) # v25.09.2
                modified_line = modified_line.replace("%generated%", generated_time)
                outfile.write(modified_line)
        os.replace("temp.h", header_file)

    except Exception as e:
        print(f"Error during file modification: {e}")
        sys.exit(1)

    end_time = datetime.datetime.now()
    duration = end_time - start_time
    print(f"Script finished at: {end_time}")
    print(f"Total execution time: {duration.total_seconds()} seconds")
    sys.exit(0)
