#!/bin/bash
# Launch Whisper64 in VICE with REU and disk image
# Usage: ./run_vice.sh [prg_file] [reu_size_kb]

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PRG="${1:-build/whisper64.prg}"
REU_SIZE="${2:-512}"
REU_IMAGE="whisper64.reu"
DISK_IMAGE="whisper64.d64"

cd "$SCRIPT_DIR"

# Check if PRG exists
if [ ! -f "$PRG" ]; then
    echo "Error: $PRG not found. Build first with: cd build && cmake -DCMAKE_PREFIX_PATH=~/llvm-mos .. && make"
    exit 1
fi

# Create REU image if it doesn't exist
if [ ! -f "$REU_IMAGE" ]; then
    echo "Creating ${REU_SIZE}KB REU image..."
    dd if=/dev/zero of="$REU_IMAGE" bs=1024 count="$REU_SIZE" 2>/dev/null
fi

# Create D64 disk image if it doesn't exist
if [ ! -f "$DISK_IMAGE" ]; then
    echo "Creating D64 disk image..."
    c1541 -format "whisper64,w6" d64 "$SCRIPT_DIR/$DISK_IMAGE"
fi

echo "Launching VICE with ${REU_SIZE}KB REU + disk image..."
echo "  PRG:  $PRG"
echo "  REU:  $REU_IMAGE (${REU_SIZE}KB)"
echo "  DISK: $DISK_IMAGE (drive 8)"

# VICE: -flag enables, +flag disables for boolean options
x64sc \
    -reu \
    -reusize "$REU_SIZE" \
    -reuimage "$SCRIPT_DIR/$REU_IMAGE" \
    -reuimagerw \
    -8 "$SCRIPT_DIR/$DISK_IMAGE" \
    -autostartprgmode 1 \
    "$SCRIPT_DIR/$PRG"
