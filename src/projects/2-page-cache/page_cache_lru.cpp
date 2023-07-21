#include "page_cache_lru.hpp"
#include "utilities/exception.hpp"

LRUReplacementPageCache::LRUReplacementPage::LRUReplacementPage(
    int argPageSize, int argExtraSize, unsigned argPageId, bool argPinned)
    : Page(argPageSize, argExtraSize), pageId(argPageId), pinned(argPinned) {}

LRUReplacementPageCache::LRUReplacementPageCache(int pageSize, int extraSize)
    : PageCache(pageSize, extraSize) {}

LRUReplacementPageCache::~LRUReplacementPageCache() {
  for (auto &[pageId, page] : pages_) {
    free(page);
  }
}

void LRUReplacementPageCache::setMaxNumPages(int maxNumPages) {
  maxNumPages_ = maxNumPages;
  /* loops through the unpinned buffer and removes until we run out unpinned
   pages or we reach the maximum number of pages allowed */
  for (auto unpinnedIt = cache_.begin(); unpinnedIt != cache_.end();) {
    if (getNumPages() <= maxNumPages_)
      break;
    LRUReplacementPage *pageToRemove = *unpinnedIt;
    // removed the unpinned page from the list
    unpinnedIt = cache_.erase(unpinnedIt);
    // removes/discards the unpinned page from the page cache
    pages_.erase(pageToRemove->pageId);
    delete pageToRemove;
  }
}

int LRUReplacementPageCache::getNumPages() const { return (int)pages_.size(); }

Page *LRUReplacementPageCache::fetchPage(unsigned pageId, bool allocate) {
  ++numFetches_;
  auto pagesIterator = pages_.find(pageId);
  /* if we find the page, pin it, and return
   a pointer to it */
  if (pagesIterator != pages_.end()) {
    ++numHits_;
    // if the page to pin is in the front of the list, remove it
    if (!cache_.empty() && cache_.front()->pageId == pageId)
      cache_.pop_front();
    pagesIterator->second->pinned = true;
    return pagesIterator->second;
  }

  // return nullptr if no page found and allocated is false
  if (!allocate)
    return nullptr;

  /* if number of pages less than the maximum allowed, allocate a new page,
   add it to the pages table, and return a pointer to it */
  if (getNumPages() < maxNumPages_) {
    LRUReplacementPage *page =
        new LRUReplacementPage(pageSize_, extraSize_, pageId, true);
    pages_.emplace(pageId, page);
    return page;
  }

  if (cache_.empty())
    return nullptr;
  /* loop through the cache list to find LRU unpinned page while also removing
   any pinned pages found along the way */
  auto cacheIt = cache_.begin();
  while (cacheIt != cache_.end()) {
    LRUReplacementPage *curr_page = *cacheIt;
    if (!curr_page->pinned)
      break;
    cacheIt = cache_.erase(cacheIt);
  }
  if (cacheIt == cache_.end())
    return nullptr;
  /* If number of pages >= max pages allowed, replace the least-recently used
   unpinned page (located the at head/front of the unpinned list) with the new
   page Id */
  LRUReplacementPage *pageToReplace = *cacheIt;
  cache_.remove(pageToReplace); // removes all occurrences of page from list
  pages_.erase(pageToReplace->pageId); // removes page's old Id from page table
  pageToReplace->pageId = pageId;      // assigns the page to the new page Id
  pageToReplace->pinned = true;        // pins the replaced page
  pages_.emplace(pageId, pageToReplace); // adds the new page ID to page table
  return pageToReplace;
}

void LRUReplacementPageCache::unpinPage(Page *page, bool discard) {
  LRUReplacementPage *pageToUnpin = (LRUReplacementPage *)page;
  /* if discard is true or there are more pages than the maximum allowed,
   discard the unpinned page */
  if (discard || getNumPages() > maxNumPages_) {
    LRUReplacementPage *pageToRemove = pageToUnpin;
    pages_.erase(pageToRemove->pageId);
    delete pageToRemove;
  } else { // otherwise, unpin the page and add to back (MRU) of unpinned list
    pageToUnpin->pinned = false;
    // if the page is already at the end of the cache list, don't add it again
    if (!cache_.empty() && cache_.back()->pageId == pageToUnpin->pageId)
      return;
    cache_.push_back(pageToUnpin);
  }
}

void LRUReplacementPageCache::changePageId(Page *page, unsigned newPageId) {
  LRUReplacementPage *pageToChange = (LRUReplacementPage *)page;
  pages_.erase(pageToChange->pageId); // removes the old page id from page table
  pageToChange->pageId = newPageId; // changed the page's ID to the new page Id

  // attempts to add the new page to the page table
  auto [pagesIterator, success] = pages_.emplace(newPageId, pageToChange);
  // if we unsuccesfully change the page ID, discard it
  if (!success) {
    delete pagesIterator->second;
    pagesIterator->second = pageToChange;
  }
}

void LRUReplacementPageCache::discardPages(unsigned pageIdLimit) {
  /* loops through all the pages in the page discard and discards any greater
   than the page Id passed in */
  for (auto pagesIterator = pages_.begin(); pagesIterator != pages_.end();) {
    if (pagesIterator->second->pageId >= pageIdLimit) {
      cache_.remove(pagesIterator->second);
      delete pagesIterator->second;
      pagesIterator = pages_.erase(pagesIterator);
    } else {
      ++pagesIterator;
    }
  }
}
