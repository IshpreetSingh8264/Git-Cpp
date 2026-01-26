#!/bin/bash

# ==========================================
# PUNJABI GIT - Quick Test Script
# Test saare commands - ek hi baar vich!
# (Test all commands - in one go!)
# ==========================================

set -e  # Fail on any error

echo "🚀 Punjabi Git - Automated Test Suite"
echo "======================================"
echo ""

# Colors for output - thoda style!
# (Colors for output - some style!)
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
GIT_BIN="$SCRIPT_DIR/build/git"

# Check if git binary exists
if [ ! -f ""$GIT_BIN"" ]; then
    echo -e "${RED}❌ Git binary not found! Build karo pehle!${NC}"
    echo -e "${RED}   (Git binary not found! Build first!)${NC}"
    echo ""
    echo "Run: cmake -S . -B build && make -C build"
    exit 1
fi

# Create test directory
TEST_DIR="/tmp/punjabi-git-test-$$"
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

echo -e "${BLUE}📁 Test directory: $TEST_DIR${NC}"
echo ""

# Test 1: Initialize repository
echo -e "${BLUE}Test 1: Repository Initialization${NC}"
""$GIT_BIN"" init
if [ -d ".git" ]; then
    echo -e "${GREEN}✅ Repository initialized${NC}"
else
    echo -e "${RED}❌ Failed to initialize repository${NC}"
    exit 1
fi
echo ""

# Test 2: Create blob object
echo -e "${BLUE}Test 2: Create Blob Object${NC}"
echo "Hello, Punjabi Git!" > test1.txt
BLOB_HASH=$("$GIT_BIN" hash-object -w test1.txt | tail -1)
echo -e "   Blob hash: ${GREEN}$BLOB_HASH${NC}"
if [ -n "$BLOB_HASH" ]; then
    echo -e "${GREEN}✅ Blob created${NC}"
else
    echo -e "${RED}❌ Failed to create blob${NC}"
    exit 1
fi
echo ""

# Test 3: Read blob object
echo -e "${BLUE}Test 3: Read Blob Object${NC}"
BLOB_CONTENT=$("$GIT_BIN" cat-file -p $BLOB_HASH | tail -1)
if [ "$BLOB_CONTENT" = "Hello, Punjabi Git!" ]; then
    echo -e "   Content: ${GREEN}$BLOB_CONTENT${NC}"
    echo -e "${GREEN}✅ Blob read correctly${NC}"
else
    echo -e "${RED}❌ Blob content mismatch${NC}"
    echo -e "   Expected: Hello, Punjabi Git!"
    echo -e "   Got: $BLOB_CONTENT"
    exit 1
fi
echo ""

# Test 4: Create multiple files and tree
echo -e "${BLUE}Test 4: Create Tree Object${NC}"
echo "File 2 content" > file2.txt
echo "File 3 content" > file3.txt
mkdir src
echo "int main() { return 0; }" > src/main.cpp
echo "void helper() {}" > src/helper.cpp

TREE_HASH=$("$GIT_BIN" write-tree | tail -1)
echo -e "   Tree hash: ${GREEN}$TREE_HASH${NC}"
if [ -n "$TREE_HASH" ]; then
    echo -e "${GREEN}✅ Tree created${NC}"
else
    echo -e "${RED}❌ Failed to create tree${NC}"
    exit 1
fi
echo ""

# Test 5: List tree contents
echo -e "${BLUE}Test 5: List Tree Contents${NC}"
"$GIT_BIN" ls-tree $TREE_HASH | tail -n +3
echo -e "${GREEN}✅ Tree listed${NC}"
echo ""

# Test 6: List tree with --name-only
echo -e "${BLUE}Test 6: List Tree (Names Only)${NC}"
"$GIT_BIN" ls-tree --name-only $TREE_HASH | tail -n +3
echo -e "${GREEN}✅ Tree names listed${NC}"
echo ""

# Test 7: Create commit
echo -e "${BLUE}Test 7: Create Commit${NC}"
export GIT_AUTHOR_NAME="Punjabi Coder"
export GIT_AUTHOR_EMAIL="coder@punjab.dev"
COMMIT_HASH=$("$GIT_BIN" commit-tree $TREE_HASH -m "Pehla commit - Test commit!" | tail -1)
echo -e "   Commit hash: ${GREEN}$COMMIT_HASH${NC}"
if [ -n "$COMMIT_HASH" ]; then
    echo -e "${GREEN}✅ Commit created${NC}"
else
    echo -e "${RED}❌ Failed to create commit${NC}"
    exit 1
fi
echo ""

# Test 8: Read commit
echo -e "${BLUE}Test 8: Read Commit${NC}"
"$GIT_BIN" cat-file -p $COMMIT_HASH | tail -n +3
echo -e "${GREEN}✅ Commit read${NC}"
echo ""

# Test 9: Create second commit with parent
echo -e "${BLUE}Test 9: Create Commit with Parent${NC}"
echo "Updated content" > file2.txt
TREE2_HASH=$("$GIT_BIN" write-tree | tail -1)
COMMIT2_HASH=$("$GIT_BIN" commit-tree $TREE2_HASH -p $COMMIT_HASH -m "Doosra commit - Second commit!" | tail -1)
echo -e "   Commit hash: ${GREEN}$COMMIT2_HASH${NC}"
if [ -n "$COMMIT2_HASH" ]; then
    echo -e "${GREEN}✅ Commit with parent created${NC}"
else
    echo -e "${RED}❌ Failed to create commit with parent${NC}"
    exit 1
fi
echo ""

# Test 10: Verify object storage
echo -e "${BLUE}Test 10: Verify Object Storage${NC}"
OBJECT_COUNT=$(find .git/objects -type f | wc -l)
echo -e "   Objects stored: ${GREEN}$OBJECT_COUNT${NC}"
if [ $OBJECT_COUNT -gt 0 ]; then
    echo -e "${GREEN}✅ Objects stored correctly${NC}"
else
    echo -e "${RED}❌ No objects found${NC}"
    exit 1
fi
echo ""

# Summary
echo "======================================"
echo -e "${GREEN}🎉 ALL TESTS PASSED!${NC}"
echo -e "${GREEN}Wadhaiya ji! Sab kaam kar reha!${NC}"
echo -e "${GREEN}(Congratulations! Everything working!)${NC}"
echo ""
echo "Test Statistics:"
echo "  - Blobs created: Multiple"
echo "  - Trees created: 2"
echo "  - Commits created: 2"
echo "  - Objects stored: $OBJECT_COUNT"
echo ""
echo "Test directory: $TEST_DIR"
echo "(You can inspect .git directory there)"
echo ""

# Cleanup option
echo "Cleanup test directory? (y/n)"
read -t 5 -n 1 CLEANUP || CLEANUP="y"
echo ""
if [ "$CLEANUP" = "y" ]; then
    rm -rf "$TEST_DIR"
    echo "✨ Cleaned up!"
else
    echo "📁 Test directory preserved: $TEST_DIR"
fi
