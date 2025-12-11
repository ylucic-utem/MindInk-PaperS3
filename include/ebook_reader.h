/**
 * @file ebook_reader.h
 * @brief Ebook/Text reader component for MindInk PaperS3
 * 
 * Extracted from display.h/cpp for modularity.
 * Handles text pagination, navigation, and font sizing.
 */

#ifndef EBOOK_READER_H
#define EBOOK_READER_H

#include <vector>
#include <WString.h>

// =============================================================================
// EBOOK READER CLASS
// =============================================================================
class EbookReader {
public:
    String fullText;
    std::vector<String> pages;
    int currentPage;
    int totalPages;
    int linesPerPage;
    int charsPerLine;
    int fontSize;  // Font size level: 0=16pt, 1=20pt, 2=24pt, 3=28pt
    
    EbookReader() : currentPage(0), totalPages(0), linesPerPage(12), charsPerLine(40), fontSize(1) {}
    
    /**
     * @brief Set text content and paginate
     * @param text Full text content
     */
    void setText(const String& text);
    
    /**
     * @brief Paginate text into pages based on current layout
     */
    void paginateText();
    
    /**
     * @brief Navigate to next page
     */
    void nextPage();
    
    /**
     * @brief Navigate to previous page
     */
    void prevPage();
    
    /**
     * @brief Check if there's a next page
     */
    bool hasNextPage();
    
    /**
     * @brief Check if there's a previous page
     */
    bool hasPrevPage();
    
    /**
     * @brief Get current page text content
     */
    String getCurrentPageText();
    
    /**
     * @brief Increase font size (max level 3)
     */
    void increaseFontSize();
    
    /**
     * @brief Decrease font size (min level 0)
     */
    void decreaseFontSize();
    
    /**
     * @brief Recalculate layout based on current font size
     */
    void recalculateLayout();
};

// =============================================================================
// GLOBAL EBOOK READER INSTANCE
// =============================================================================
extern EbookReader ebookReader;

// =============================================================================
// EBOOK DISPLAY FUNCTIONS
// =============================================================================

/**
 * @brief Draw current ebook page with navigation controls
 */
void drawEbookPage();

/**
 * @brief Handle touch events on ebook page
 * @param x Touch X coordinate
 * @param y Touch Y coordinate
 * @return true if handled within ebook, false if BACK pressed
 */
bool handleEbookTouch(int x, int y);

#endif // EBOOK_READER_H
