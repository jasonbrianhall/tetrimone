#!/bin/bash

# Variables
DJGPP_IMAGE="djfdyuruiry/djgpp"
ALLEGRO_GIT="https://github.com/liballeg/allegro5.git"
ALLEGRO_TAG="4.4.3"  # Using 4.x for DOS compatibility
CSDPMI_URL="http://na.mirror.garr.it/mirrors/djgpp/current/v2misc/csdpmi7b.zip"
USER_ID=$(id -u)
GROUP_ID=$(id -g)
DOS_TARGET="tetrimone.exe"

# Check and compile Linux version if g++ is available
if command -v g++ &> /dev/null; then
    echo "Compiling Linux version..."
    g++ tetrimone.cpp -lallegro -lallegro_primitives -lallegro_font -lallegro_ttf -o tetrimone
else
    echo "g++ not found - skipping Linux build"
fi

# Download CSDPMI if needed
if [ ! -d "csdpmi" ]; then
    echo "Downloading CSDPMI..."
    wget ${CSDPMI_URL}
    mkdir -p csdpmi
    unzip -o csdpmi7b.zip -d csdpmi
fi

# Create a script to run inside Docker for building Allegro and the application
cat > build_msdos.sh << 'EOF'
#!/bin/bash
set -e

echo "Installing cmake"
apt-get update
apt-get install cmake

# Clone Allegro
cd /tmp
echo "Cloning Allegro..."
git clone https://github.com/liballeg/allegro5.git -b 4.4.3
cd allegro5

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring Allegro with CMake..."
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-djgpp.cmake \
         -DWANT_DOCS=OFF \
         -DWANT_EXAMPLES=OFF \
         -DWANT_TESTS=OFF \
         -DCMAKE_INSTALL_PREFIX=/tmp/allegro_install

# Build Allegro
echo "Building Allegro..."
make -j4
make install

# Build the Tetris application
cd /src
echo "Building Tetris for MSDOS..."

# Include paths for Allegro
echo "Compiling Tetris..."
g++ tetrimone.cpp -o tetrimone.exe -I/tmp/allegro_install/include -L/tmp/allegro_install/lib -lalleg -DMSDOS

echo "Build complete!"
EOF

# Make the script executable
chmod +x build_msdos.sh

# Create a temporary directory to avoid symlink issues
TEMP_BUILD_DIR=$(mktemp -d)
echo "Created temporary build directory: ${TEMP_BUILD_DIR}"

# Copy tetrimone.cpp directly
cp -L tetrimone.cpp "${TEMP_BUILD_DIR}/"
cp build_msdos.sh "${TEMP_BUILD_DIR}/"

# List files in the temporary directory for verification
echo "Files in temporary build directory:"
ls -la "${TEMP_BUILD_DIR}"

# Run the Docker container with the temporary directory
# Note: We're using --network=host to allow Git access
echo "Starting Docker build process..."
docker run --rm --network=host -v "${TEMP_BUILD_DIR}:/src:z" -u ${USER_ID}:${GROUP_ID} ${DJGPP_IMAGE} /src/build_msdos.sh

# Copy back the built files
echo "Copying built files from temporary directory..."
cp "${TEMP_BUILD_DIR}/${DOS_TARGET}" ./ 2>/dev/null || echo "Failed to copy executable"

# Clean up
echo "Cleaning up temporary directory..."
rm -rf "${TEMP_BUILD_DIR}"

# Ensure we have the CSDPMI executable in the current directory
if [ -f "csdpmi/bin/CWSDPMI.EXE" ]; then
    cp csdpmi/bin/CWSDPMI.EXE .
fi

# Check if build was successful
if [ -f "${DOS_TARGET}" ]; then
    echo "MSDOS build successful! Files created:"
    echo "- ${DOS_TARGET}"
    echo "- CWSDPMI.EXE"
    echo "To run in DOSBox, execute: dosbox ${DOS_TARGET}"
else
    echo "MSDOS build failed."
fi
