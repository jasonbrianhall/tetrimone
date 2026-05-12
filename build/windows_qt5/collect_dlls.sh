#!/bin/bash

# Help function
show_help() {
    echo "Usage: $0 EXE_PATH DLL_SOURCE_DIR OUTPUT_DIR"
    echo
    echo "Collects required DLLs for a Windows executable."
    echo
    echo "Parameters:"
    echo "  EXE_PATH        Path to the Windows executable"
    echo "  DLL_SOURCE_DIR  Directory containing source DLLs"
    echo "  OUTPUT_DIR      Directory where DLLs will be copied"
    exit 1
}

# Check parameters
if [ $# -ne 3 ]; then
    show_help
fi

EXE_PATH="$1"
DLL_SOURCE_DIR="$2"
OUTPUT_DIR="$3"

# Verify inputs
if [ ! -f "$EXE_PATH" ]; then
    echo "Error: Executable not found: $EXE_PATH"
    exit 1
fi

if [ ! -d "$DLL_SOURCE_DIR" ]; then
    echo "Error: DLL directory not found: $DLL_SOURCE_DIR"
    exit 1
fi

if [ ! -d "$OUTPUT_DIR" ]; then
    echo "Error: Output directory not found: $OUTPUT_DIR"
    exit 1
fi

# Derive Qt5 directory from DLL directory (typically /mingw64/bin -> /mingw64)
QT5_DIR=$(dirname "$DLL_SOURCE_DIR")

# Function to get direct dependencies of a file
get_dependencies() {
    local file="$1"
    objdump -p "$file" | grep "DLL Name:" | sed 's/\s*DLL Name: //'
}

# Function to check if a DLL exists in source directory
dll_exists() {
    local dll="$1"
    [ -f "$DLL_SOURCE_DIR/$dll" ]
}

# Initialize arrays for processing
declare -A processed_dlls
declare -a dlls_to_process
declare -a missing_dlls

# Create necessary Qt5 directories
echo "Creating Qt5 directory structure..."
mkdir -p "$OUTPUT_DIR/share/glib-2.0/schemas"
mkdir -p "$OUTPUT_DIR/share/icons"
mkdir -p "$OUTPUT_DIR/share/themes"
mkdir -p "$OUTPUT_DIR/lib/gdk-pixbuf-2.0"
mkdir -p "$OUTPUT_DIR/lib/gtk-3.0"

# Copy Qt5 related files
echo "Copying Qt5 files..."
echo "Looking for schemas in: $QT5_DIR/share/glib-2.0/schemas/"
ls -l "$QT5_DIR/share/glib-2.0/schemas/" || echo "Directory not found"

# Try multiple possible locations for gschemas.compiled
SCHEMA_LOCATIONS=(
    "$QT5_DIR/share/glib-2.0/schemas/gschemas.compiled"
    "$DLL_SOURCE_DIR/../share/glib-2.0/schemas/gschemas.compiled"
    "/usr/x86_64-w64-mingw32/sys-root/mingw/share/glib-2.0/schemas/gschemas.compiled"
    "/mingw64/share/glib-2.0/schemas/gschemas.compiled"
)

SCHEMA_FOUND=0
for schema_path in "${SCHEMA_LOCATIONS[@]}"; do
    echo "Checking for schema at: $schema_path"
    if [ -f "$schema_path" ]; then
        echo "Found schema file at: $schema_path"
        cp "$schema_path" "$OUTPUT_DIR/share/glib-2.0/schemas/"
        SCHEMA_FOUND=1
        break
    fi
done

if [ $SCHEMA_FOUND -eq 0 ]; then
    echo "Warning: Could not find gschemas.compiled in any of the expected locations"
fi

# Copy theme-related files if they exist
if [ -d "$QT5_DIR/share/icons/hicolor" ]; then
    cp -r "$QT5_DIR/share/icons/hicolor" "$OUTPUT_DIR/share/icons/"
fi
if [ -d "$QT5_DIR/share/themes/default" ]; then
    cp -r "$QT5_DIR/share/themes/default" "$OUTPUT_DIR/share/themes/"
fi
if [ -d "$QT5_DIR/share/themes/emacs" ]; then
    cp -r "$QT5_DIR/share/themes/emacs" "$OUTPUT_DIR/share/themes/"
fi

# Create GTK settings file
cat > "$OUTPUT_DIR/settings.ini" << EOF
[Settings]
gtk-theme-name = default
gtk-icon-theme-name = hicolor
gtk-font-name = Segoe UI 9
gtk-menu-images = true
gtk-button-images = true
gtk-toolbar-style = both-horiz
EOF

# Copy Qt5 plugins - specifically copy platforms directory to root
echo "Copying Qt5 platforms plugin..."

QT5_PLUGIN_DIR="/usr/x86_64-w64-mingw32/sys-root/mingw/lib/qt5/plugins"

if [ -d "$QT5_PLUGIN_DIR/platforms" ]; then
    echo "Found Qt5 platforms at: $QT5_PLUGIN_DIR/platforms/"
    cp -r "$QT5_PLUGIN_DIR/platforms" "$OUTPUT_DIR/"
    echo "✓ Qt5 platforms directory copied to root"
else
    echo "ERROR: Could not find Qt5 platforms at $QT5_PLUGIN_DIR/platforms"
    exit 1
fi

# Ensure platforms directory exists with correct permissions
mkdir -p "$OUTPUT_DIR/platforms"

# Copy Qt5 configuration files if they exist
if [ -f "$QT5_DIR/bin/qt.conf" ]; then
    cp "$QT5_DIR/bin/qt.conf" "$OUTPUT_DIR/"
fi

# Explicitly copy SDL3.dll first
echo "Copying SDL3.dll explicitly..."
SDL3_SRC="$DLL_SOURCE_DIR/SDL3.dll"
echo "Looking for SDL3.dll at: $SDL3_SRC"
if [ -f "$SDL3_SRC" ]; then
    echo "Found SDL3.dll, copying to $OUTPUT_DIR"
    cp "$SDL3_SRC" "$OUTPUT_DIR/"
    processed_dlls["SDL3.dll"]=1
    echo "✓ Copied SDL3.dll to output directory"
else
    echo "✗ WARNING: SDL3.dll not found at $SDL3_SRC"
fi



# Add other critical DLLs to processing queue
CRITICAL_DLLS=(
    "SDL2.dll"
    "Qt5Core.dll"
    "Qt5Gui.dll"
    "Qt5Widgets.dll"
    "libwinpthread-1.dll"
)

echo "Checking for other critical DLLs..."
for dll in "${CRITICAL_DLLS[@]}"; do
    if dll_exists "$dll"; then
        if [ "${processed_dlls[$dll]}" != "1" ]; then
            echo "Adding critical DLL: $dll"
            dlls_to_process+=("$dll")
        fi
    else
        echo "Note: Critical DLL not found: $dll"
    fi
done

# Get initial dependencies
echo "Analyzing dependencies for $(basename "$EXE_PATH")..."
initial_dlls=$(get_dependencies "$EXE_PATH")

# Add initial DLLs to processing queue
for dll in $initial_dlls; do
    if dll_exists "$dll"; then
        dlls_to_process+=("$dll")
    else
        missing_dlls+=("$dll")
    fi
done

# Process DLLs recursively
while [ ${#dlls_to_process[@]} -gt 0 ]; do
    current_dll="${dlls_to_process[0]}"
    dlls_to_process=("${dlls_to_process[@]:1}")
    
    # Skip if already processed
    [ "${processed_dlls[$current_dll]}" = "1" ] && continue
    
    echo "Processing: $current_dll"
    
    # Mark as processed
    processed_dlls[$current_dll]=1
    
    # Copy DLL to output directory
    cp "$DLL_SOURCE_DIR/$current_dll" "$OUTPUT_DIR/"
    
    # Get dependencies of current DLL
    subdeps=$(get_dependencies "$DLL_SOURCE_DIR/$current_dll")
    
    # Add new dependencies to processing queue
    for dll in $subdeps; do
        if [ "${processed_dlls[$dll]}" != "1" ]; then
            if dll_exists "$dll"; then
                dlls_to_process+=("$dll")
            else
                if [[ ! " ${missing_dlls[@]} " =~ " ${dll} " ]]; then
                    missing_dlls+=("$dll")
                fi
            fi
        fi
    done
done

# Print summary
echo -e "\nDLL Collection Complete!"
echo "Copied ${#processed_dlls[@]} DLLs"

if [ ${#missing_dlls[@]} -gt 0 ]; then
    echo -e "\nNote: The following DLLs were not found in the source directory:"
    printf '%s\n' "${missing_dlls[@]}"
    echo "These are likely system DLLs that will be present on the target Windows system."
fi

echo -e "\nDirectory structure created:"
echo "  $OUTPUT_DIR/plugins/ - Qt5 plugins (platforms, imageformats, etc.)"
echo "  $OUTPUT_DIR/share/   - GTK resources"
echo "  $OUTPUT_DIR/        - DLLs and executable"
