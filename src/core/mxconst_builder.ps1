# --- Configuration ---
#  > Enable PowerShell script execution.
#  Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
################################ 

$className = "mxconst"
$templateFile = "mxconst.template"
$headerFile = "mxconst.h"
#$cppFile = "mxconst.cpp" # Not directly used, so commented out
$startTime = Get-Date -Format s

# --- Functions ---

function Write-MxconstHeaderFooter {
    Add-Content -Path $headerFile -Value "`n`n`t};// end class `n} // missionx namespace"
    Add-Content -Path $headerFile -Value "`n#endif //MXCONST_H"
}

function Write-HeaderLineConstexpr {
    param(
        [string]$definition,
        [string]$dataType,
        [string]$name,
        [string]$paramValue,
        [string]$comment
    )

    $name = $name.Trim()
    $headerLine = "$definition $dataType $name = $paramValue"
    $trimmedValue = $paramValue.TrimEnd()
    $semicolon = ""
    if ($trimmedValue -notlike "*;") {
        $semicolon = ";"
    }

    # Remove " const " from header line only if "name" does not start with "*" (pointer)
    if ($name -notlike "\*`*") { #escaped the * char
        $headerLine = $headerLine -replace " const ", ""
    }

    $fullLine = "$headerLine$semicolon"
    if ($comment) {
        $fullLine = "$fullLine // $comment"
    }
    Add-Content -Path $headerFile -Value $fullLine
}

function Write-HeaderStruct {
    param(
        [string]$dataType,
        [string]$name,
        [string]$paramValue
    )

    $name = $name.Trim()
    $structName = "st_$($name.ToLower())"
    $getterName = "get_$($name)"
    $trimmedValue = $paramValue.TrimEnd()
    $semicolon = ""

    if ($trimmedValue -notlike "*;") {
        $semicolon = ";"
    }

    Add-Content -Path $headerFile -Value "// ---> $name"
    Add-Content -Path $headerFile -Value "typedef struct $structName {"
    Add-Content -Path $headerFile -Value "  $dataType value = $paramValue$semicolon;"
    Add-Content -Path $headerFile -Value "[[nodiscard]]  $dataType getValue() const { return value; }"
    Add-Content -Path $headerFile -Value "} $structName;"
    # Add space
    # Create the getter function
    Add-Content -Path $headerFile -Value "// -- getter -- "
    Add-Content -Path $headerFile -Value "static $dataType $getterName() {"
    Add-Content -Path $headerFile -Value "  static const $structName instance;"
    Add-Content -Path $headerFile -Value "  return instance.getValue();"
    Add-Content -Path $headerFile -Value "}"
    Add-Content -Path $headerFile -Value "// $name <--`n"
}

# --- Main Script ---
function Main {
    Write-Output "Starting script at: $(Get-Date)"

    # Reset files
    Write-Output "Resetting file: $headerFile"
    Set-Content -Path $headerFile -Value ""
    #Set-Content -Path $cppFile -Value "" # Not used

    Write-Output "Processing template file: $templateFile"
    Get-Content -Path $templateFile | ForEach-Object {
        $line = $_.Trim()
        if ($line -notmatch "^(const|static|constexpr|inline|>).*") {
            #Write-Output "Appending to header (non-keyword): $line"
            Add-Content -Path $headerFile -Value $line
        }
        else {
            # Split the line using "^" as delimiter
            $fields = $line.Split("^")

            # Check if we have at least 4 fields
            if ($fields.Length -lt 4) {
                Write-Warning "Line '$line' has less than 4 fields. Skipping: $line"
                continue
            }

            $definition = $fields[0]
            $dataType = $fields[1]
            $name = $fields[2]
            $paramValue = $fields[3]
            $comment = ($fields | Select-Object -Skip 4) -join "^" # Rejoin any remaining parts of the comment

            # Handle constexpr
            if ($definition -like "*constexpr*") {
                $modifiedDefinition = $definition
                if ($definition -notmatch "static") {
                    $modifiedDefinition = "static $modifiedDefinition"
                }
                Write-HeaderLineConstexpr $modifiedDefinition $dataType $name $paramValue $comment
            }
            # Handle static (without constexpr)
            elseif ($definition -like "*static*" -and $definition -notlike "*constexpr*") {
                Write-HeaderStruct $dataType $name $paramValue
            }
            else {
                # For other keywords (const, inline, >) just write to header
                $headerLine = "$definition $dataType $name"
                $semicolon = ""
                $trimmedValue = $paramValue.TrimEnd()
                if ($trimmedValue -notlike "*;") {
                    $semicolon = ";"
                }
                $fullLine = "$headerLine$semicolon"
                if ($comment) {
                    $fullLine = "$fullLine // $comment"
                }
                Add-Content -Path $headerFile -Value $fullLine
            }
        }
    }
}

Main

Write-MxconstHeaderFooter

# Replace "%class_name%" with "$className" in mxconst.h
Write-Output "Replacing '%class_name%' with '$className' in $headerFile"
(Get-Content -Path $headerFile) -replace "%class_name%",$className | Set-Content -Path $headerFile
# (Get-Content -Path $headerFile) -replace "%generated%",(Get-Date) | Set-Content -Path $headerFile
(Get-Content -Path $headerFile) -replace "%generated%",(Get-Date -Format "yyyy MMM dd, HH:mm:ss") | Set-Content -Path $headerFile

$endTime = Get-Date -Format s
$duration = New-TimeSpan -Start $startTime -End $endTime
Write-Output "Script finished at: $(Get-Date)"
Write-Output "Total execution time: $($duration.TotalSeconds) seconds"

exit
