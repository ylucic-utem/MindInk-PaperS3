#!/bin/bash
# MindInk Test Script Setup - One-Click Installation

echo "================================================"
echo "  MindInk Local Processing Test Setup"
echo "================================================"
echo ""

# Check if Python is installed
if ! command -v python3 &> /dev/null; then
    echo "❌ Python 3 is not installed"
    echo "   Please install Python 3.8 or higher from python.org"
    exit 1
fi

echo "✅ Python 3 found: $(python3 --version)"
echo ""

# Check if pip is installed
if ! command -v pip3 &> /dev/null; then
    echo "❌ pip3 is not installed"
    echo "   Please install pip or upgrade Python"
    exit 1
fi

echo "✅ pip3 found"
echo ""

# Install dependencies
echo "Installing Python dependencies..."
pip3 install -r requirements.txt

if [ $? -ne 0 ]; then
    echo "❌ Failed to install dependencies"
    exit 1
fi

echo "✅ Dependencies installed"
echo ""

# Create .env if it doesn't exist
if [ ! -f ".env" ]; then
    echo "Creating .env file from template..."
    cp .env.example .env
    echo "✅ .env file created"
    echo ""
    echo "⚠️  IMPORTANT: Edit .env and add your API keys:"
    echo "   ELEVEN_LABS_API_KEY=sk_..."
    echo "   GEMINI_API_KEY=AIza_..."
else
    echo "✅ .env file already exists"
fi

# Create audio_cache directory
mkdir -p audio_cache
echo "✅ Cache directory created: ./audio_cache/"
echo ""

# Final instructions
echo "================================================"
echo "  Setup Complete! ✅"
echo "================================================"
echo ""
echo "Next steps:"
echo "  1. Edit .env and add your API keys"
echo "  2. Run: python3 test_local_processing.py"
echo ""
echo "For help, see:"
echo "  - QUICKSTART_REFERENCE.md"
echo "  - TEST_SCRIPT_README.md"
echo ""
