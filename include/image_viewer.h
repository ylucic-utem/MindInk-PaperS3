/**
 * @file image_viewer.h
 * @brief Image viewing and gallery functions for MindInk PaperS3
 * 
 * Extracted from display.cpp for modularity.
 * Handles image display, gallery views, and infographic rendering.
 */

#ifndef IMAGE_VIEWER_H
#define IMAGE_VIEWER_H

#include <vector>
#include <WString.h>
#include "storage.h"

// =============================================================================
// IMAGE DISPLAY FUNCTIONS
// =============================================================================

/**
 * @brief Draw gallery grid view
 * @param images Vector of image files to display
 */
void drawGallery(const std::vector<ImageFile>& images);

/**
 * @brief Draw summaries list for infographic selection
 * @param summaries Vector of summary files
 */
void drawSummariesForInfographic(const std::vector<SummaryFile>& summaries);

/**
 * @brief Draw infographics grid from SD card
 * @param infographics Vector of infographic filenames
 */
void drawInfographicsOnSD(const std::vector<String>& infographics);

/**
 * @brief Draw a message with image styling
 * @param title Title text
 * @param message Message text
 */
void drawImageMessage(const String& title, const String& message);

/**
 * @brief Draw image from file path
 * @param label Display label
 * @param path File path on SD card
 */
void drawImageFromFile(const String& label, const String& path);

/**
 * @brief Draw image from memory buffer
 * @param label Display label
 * @param data Image data buffer
 * @param len Data length
 */
void drawImageFromBuffer(const String& label, const uint8_t* data, size_t len);

#endif // IMAGE_VIEWER_H
